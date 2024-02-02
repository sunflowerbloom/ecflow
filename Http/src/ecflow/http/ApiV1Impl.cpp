/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/http/ApiV1Impl.hpp"

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "ecflow/core/Child.hpp"
#include "ecflow/core/Converter.hpp"
#include "ecflow/core/Str.hpp"
#include "ecflow/http/BasicAuth.hpp"
#include "ecflow/http/HttpServerException.hpp"
#include "ecflow/http/Options.hpp"
#if defined(ECF_OPENSSL)
    #include "ecflow/http/TokenStorage.hpp"
#endif
#include "ecflow/http/TypeToJson.hpp"
#include "ecflow/node/Defs.hpp"
#include "ecflow/node/Family.hpp"
#include "ecflow/node/Limit.hpp"
#include "ecflow/node/Suite.hpp"
#include "ecflow/node/parser/DefsStructureParser.hpp"

// For token based authentication, this user account
// needs to have full access to server as it's acting
// in lieu of the actual user

namespace ecf::http {

const char* ECF_USER = getenv("ECF_USER");
const char* ECF_PASS = getenv("ECF_PASS");

extern std::atomic<unsigned int> last_request_time;

std::shared_ptr<Defs> defs_ = nullptr;

static std::mutex def_mutex, cv_mutex;
static std::condition_variable defs_cv;
std::atomic<bool> update_defs(true);

namespace {

static void print_polling_interval_notification(long long sleeptime,
                                                long long base_sleeptime,
                                                long long drift,
                                                long long max_sleeptime) {
    const char* fmt = "Polling interval is now %lld (base: %lld drift: %lld max: %lld)\n";
    printf(fmt, sleeptime, base_sleeptime, drift, max_sleeptime);
}

template <typename T>
void trigger_defs_update(T&& func) {
    {
        std::lock_guard<std::mutex> lock(cv_mutex);
        update_defs = true;
        defs_cv.notify_one();
    }

    while (update_defs.load() == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    func();
}

void trigger_defs_update() {
    trigger_defs_update([] {});
}

std::string tolower(const std::string& str) {
    std::string ret = str;
    for (auto& c : ret) {
        c = std::tolower(c);
    }
    return ret;
}

std::string json_type_to_string(const ojson& j) {
    switch (j.type()) {
        case ojson::value_t::null:
            return "null";
        case ojson::value_t::boolean:
            return (j.get<bool>()) ? "true" : "false";
        case ojson::value_t::string:
            return j.get<std::string>();
        case ojson::value_t::binary:
            return j.dump();
        case ojson::value_t::array:
        case ojson::value_t::object:
            return j.dump();
        case ojson::value_t::discarded:
            return "discarded";
        case ojson::value_t::number_integer:
            return ecf::convert_to<std::string>(j.get<int>());
        case ojson::value_t::number_unsigned:
            return ecf::convert_to<std::string>(j.get<unsigned int>());
        case ojson::value_t::number_float:
            return ecf::convert_to<std::string>(j.get<double>());
        default:
            return std::string();
    }
}

} // namespace

std::shared_ptr<Defs> get_defs() {
    std::lock_guard<std::mutex> lock(def_mutex);
    return defs_;
}

ojson make_node_json(node_ptr node) {
    return ojson::object({{"type", tolower(node->debugType())}, {"name", node->name()}, {"children", ojson::array()}});
}

void print_sparser_node(ojson& j, const std::vector<node_ptr>& nodes) {
    for (const auto& node : nodes) {
        const std::string type = tolower(node->debugType());

        j[node->name()] = ojson::object({});
        if (type == "family") {
            print_sparser_node(j[node->name()], std::dynamic_pointer_cast<Family>(node)->nodeVec());
        }
    }
}

void print_node(ojson& j, const std::vector<node_ptr>& nodes, bool add_id = false) {
    j["children"].get_ptr<ojson::array_t*>()->reserve(nodes.size());
    for (const auto& node : nodes) {
        const std::string type = tolower(node->debugType());
        auto jnode             = make_node_json(node);

        j["children"].push_back(jnode);
        if (type == "family") {
            print_node(j["children"].back(), std::dynamic_pointer_cast<Family>(node)->nodeVec(), add_id);
        }
    }
}

bool authenticate(const httplib::Request& request, ClientInvoker* ci) {

    auto auth_with_token = [&](const std::string& token) -> bool {
#ifdef ECF_OPENSSL
        if (TokenStorage::instance().verify(token)) {
            if (ECF_USER != nullptr && ECF_PASS != nullptr) {
                ci->set_user_name(std::string(ECF_USER));
                ci->set_password(std::string(ECF_PASS));
            }
            return true;
        }
#endif
        return false;
    };

    const auto& header = request.headers;

    // Authorization: Basic ..........
    // Authorization: Bearer ..........

    auto auth = header.find("Authorization");

    if (auth != header.end()) {
        std::vector<std::string> elems;
        ecf::Str::split(auth->second, elems, " ");

        if (elems[0] == "Basic") {
            auto creds = BasicAuth::get_credentials(elems[1]);
            ci->set_user_name(creds.first);
            ci->set_password(creds.second);
            return true;
        }
        else if (elems[0] == "Bearer") {
#ifndef ECF_OPENSSL
            throw HttpServerException(HttpStatusCode::server_error_internal_server_error,
                                      "Server compiled without SSL support");
#else
            return auth_with_token(elems[1]);
#endif
        }
        return false;
    }

    // No standard authentication header set,
    // try X-API-Key and/or queryparam

    auth = header.find("X-API-Key");

    if (auth != header.end()) {
#ifndef ECF_OPENSSL
        throw HttpServerException(HttpStatusCode::server_error_internal_server_error,
                                  "Server compiled without SSL support");
#else
        return auth_with_token(auth->second);
#endif
    }

    // allow token to be passed as a query parameter
    const std::string token = request.get_param_value("key");

    if (token.empty() == false) {
#ifndef ECF_OPENSSL
        throw HttpServerException(HttpStatusCode::server_error_internal_server_error,
                                  "Server compiled without SSL support");
#else
        return auth_with_token(token);
#endif
    }

    return false;
}

std::unique_ptr<ClientInvoker> get_client(const httplib::Request& request) {
    auto ci = std::make_unique<ClientInvoker>();

    if (request.method != "GET" && request.method != "OPTIONS" && request.method != "HEAD" &&
        authenticate(request, ci.get()) == false) {
        throw HttpServerException(HttpStatusCode::client_error_unauthorized, "Unauthorized");
    }
    return ci;
}

std::unique_ptr<ClientInvoker> get_client_for_tasks(const httplib::Request& request, const ojson& payload) {
    auto ci = get_client(request);

    ci->set_child_path(payload.at("ECF_NAME").get<std::string>());
    ci->set_child_password(payload.at("ECF_PASS").get<std::string>());
    ci->set_child_pid(json_type_to_string(payload.at("ECF_RID")));
    ci->set_child_try_no(std::stoi(json_type_to_string(payload.at("ECF_TRYNO"))));
    ci->set_child_timeout(payload.value("ECF_TIMEOUT", 86400));
    ci->set_zombie_child_timeout(payload.value("ECF_ZOMBIE_TIMEOUT", 43200));

    return ci;
}

node_ptr get_node(const std::string& path) {
    node_ptr node = get_defs()->findAbsNode(path);
    if (node.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }

    return node;
}

ojson get_node_status(const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto client = get_client(request);
    client->query("dstate", path, "");

    ojson j;
    j["status"] = client->server_reply().get_string();
    j["path"]   = path;

    node_ptr node = get_node(path);

    if (node) {
        j["default_status"] = DState::to_string(node->defStatus());

        std::vector<std::string> why;
        node->bottom_up_why(why);
        j["why"]                  = ojson::object({});
        j["why"]["bottom_up_why"] = why;
        why.clear();
        node->top_down_why(why);
        j["why"]["top_down_why"] = why;
    }

    return j;
}

template <typename T>
void apply_to_parents(const Node* node, T&& func) {
    for (const Node* up = node; up != nullptr; up = up->parent()) {
        func(up);
    }
}

ojson get_sparser_node_tree(const std::string& path) {
    ojson j;

    if (path == "/") {
        const std::vector<suite_ptr> suites = get_defs()->suiteVec();
        for (const auto& suite : suites) {
            j[suite->name()] = ojson::object({});

            print_sparser_node(j[suite->name()], suite->nodeVec());
        }
    }
    else {
        node_ptr node = get_node(path);

        if (node == nullptr) {
            throw HttpServerException(HttpStatusCode::client_error_not_found, "Node " + path + " not found");
        }

        const std::string type = tolower(node->debugType());

        if (type == "family") {
            print_sparser_node(j, std::dynamic_pointer_cast<Family>(node)->nodeVec());
        }
        else if (type == "suite") {
            print_sparser_node(j, std::dynamic_pointer_cast<Suite>(node)->nodeVec());
        }
    }

    return j;
}

ojson get_node_tree(const std::string& path, bool add_id = false) {
    ojson j;

    if (path == "/") {
        const std::vector<suite_ptr> suites = get_defs()->suiteVec();
        j["suites"]                         = ojson::array();
        j["suites"].get_ptr<ojson::array_t*>()->reserve(suites.size());

        for (const auto& suite : suites) {
            ojson js = make_node_json(suite);

            print_node(js, suite->nodeVec(), add_id);

            j["suites"].push_back(js);
        }
    }
    else {
        node_ptr node = get_node(path);

        if (node == nullptr)
            throw HttpServerException(HttpStatusCode::client_error_not_found, "Node " + path + " not found");

        j = make_node_json(node);

        const std::string type = tolower(node->debugType());

        if (type == "family") {
            print_node(j, std::dynamic_pointer_cast<Family>(node)->nodeVec(), add_id);
        }
        else if (type == "suite") {
            print_node(j, std::dynamic_pointer_cast<Suite>(node)->nodeVec(), add_id);
        }
    }

    return j;
}

ojson get_suites() {
    auto suites = get_defs()->suiteVec();

    ojson j = ojson::array();
    for (const auto& s : suites) {
        j.push_back(s->name());
    }
    return j;
}

ojson get_server_attributes() {

    ojson j;

    j["variables"] = get_defs()->server().user_variables();

    for (auto v : get_defs()->server().server_variables()) {
        ojson _j    = v;
        _j["const"] = true;
        j["variables"].push_back(_j);
    }
    return j;
}

ojson get_node_attributes(const std::string& path) {
    ojson j;

    node_ptr node = get_node(path);

    j["meters"]      = node->meters();
    j["limits"]      = node->limits();
    j["inlimits"]    = node->inlimits();
    j["events"]      = node->events();
    j["variables"]   = node->variables();
    j["labels"]      = node->labels();
    j["dates"]       = node->dates();
    j["days"]        = node->days();
    j["crons"]       = node->crons();
    j["times"]       = node->timeVec();
    j["todays"]      = node->todayVec();
    j["repeat"]      = node->repeat();
    j["path"]        = path;
    j["trigger"]     = node->get_trigger();
    j["complete"]    = node->get_complete();
    j["flag"]        = node->get_flag();
    j["late"]        = node->get_late();
    j["zombies"]     = node->zombies();
    j["generics"]    = node->generics();
    j["queues"]      = node->queues();
    j["autocancel"]  = node->get_autocancel();
    j["autoarchive"] = node->get_autoarchive();
    j["autorestore"] = node->get_autorestore();

    {
        // Collect 'normal' variables
        auto vars = node->variables();
        // ... and generated variables
        node->gen_variables(vars);

        j["variables"] = vars;
    }

    apply_to_parents(node->parent(), [&j](const Node* n) {
        // Collect 'normal' variables
        auto vars = n->variables();
        // ... and generated variables
        n->gen_variables(vars);

        j["inherited_variables"][n->name()] = vars;
    });

    auto server_variables = get_defs()->server().server_variables();
    auto user_variables   = get_defs()->server().user_variables();

    server_variables.insert(server_variables.end(), user_variables.begin(), user_variables.end());

    j["inherited_variables"]["server"] = server_variables;

    return j;
}

ojson get_node_definition(const std::string& path) {
    ojson j;

    node_ptr node = get_node(path);

    j["definition"] = node->print();
    j["path"]       = path;

    return j;
}

ojson get_node_output(const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto client = get_client(request);

    ojson j;

    client->file(path, "jobout");
    j["job_output"] = client->server_reply().get_string();

    try {
        client->file(path, "kill");
        j["kill_output"] = client->server_reply().get_string();
    }
    catch (const std::exception& e) {
        j["kill_output"] = "";
    }

    try {
        client->file(path, "stat");
        j["status_output"] = client->server_reply().get_string();
    }
    catch (const std::exception& e) {
        j["status_output"] = "";
    }

    return j;
}

void add_suite(const httplib::Request& request, httplib::Response& response) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string defs = payload.at("definition");

    DefsStructureParser parser(defs);

    std::string errorMsg, warningMsg;

    if (parser.doParse(errorMsg, warningMsg) == false) {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Invalid definition: " + errorMsg + "/" + warningMsg);
    }

    node_ptr node = parser.the_node_ptr();

    auto defsptr = Defs::create();
    errorMsg = "", warningMsg = "";
    defsptr->restore_from_string(defs, errorMsg, warningMsg);

    try {
        if (json_type_to_string(payload.at("auto_add_externs")) == "true") {
            defsptr->auto_add_externs(true);
        }
    }
    catch (const ojson::exception& e) {
    }

    auto client = get_client(request);
    client->load(defsptr);

    ojson j;
    j["message"] = "Suite(s) created successfully";
    response.set_content(j.dump(), "application/json");
    response.status = HttpStatusCode::success_created;
    response.set_header("Location", request.path + "/" + node->name() + "/definition");
}

ojson update_node_definition(const httplib::Request& request) {
    const std::string path         = request.matches[1];
    const ojson payload            = ojson::parse(request.body);
    const std::string new_defs_str = payload.at("definition");
    const bool force               = payload.value("force", false);

    // Support "partial node adding/replacement", meaning that
    // user can give us just the definition of a new task, and
    // we will add that to the suite.

    // this takes a long time, but we need a copy as we are changing
    // the defs locally here

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path not found");
    }

    DefsStructureParser parser(new_defs_str);

    std::string errorMsg, warningMsg;

    if (parser.doParse(errorMsg, warningMsg) == false) {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Invalid definition: " + errorMsg + "/" + warningMsg);
    }

    node_ptr node = parser.the_node_ptr();

    size_t pos = 0;

    // If a child with the same name exists already, it needs to be removed
    // before adding it with this new definition
    auto existing_node = parent->findImmediateChild(node->name(), pos);

    if (existing_node.get() != nullptr) {
        parent->removeChild(existing_node.get());
    }

    if (parent->isAddChildOk(node.get(), errorMsg) == false) {
        throw HttpServerException(HttpStatusCode::client_error_bad_request, errorMsg);
    }

    parent->addChild(node);

    auto client = get_client(request);
    client->replace(path, defs->print(), true, force);

    ojson j;
    j["path"]    = path;
    j["message"] = "Definition updated successfully";
    return j;
}

template <typename T>
void add_attribute_to_node(node_ptr node, const T& attr);

template <>
void add_attribute_to_node(node_ptr node, const Meter& attr) {
    node->addMeter(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const Event& attr) {
    node->addEvent(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::CronAttr& attr) {
    node->addCron(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const std::string& attr) {
    node->add_complete(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::AutoCancelAttr& attr) {
    node->addAutoCancel(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::AutoRestoreAttr& attr) {
    node->add_autorestore(attr);
}

template <>
void add_attribute_to_node(node_ptr node, const ecf::AutoArchiveAttr& attr) {
    node->add_autoarchive(attr);
}

template <typename T>
void add_attribute_to_path(const T& attr, const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }
    add_attribute_to_node(parent, attr);

    auto client = get_client(request);
    client->replace(path, defs->print(), false, false);
}

void remove_attribute_from_path(const std::string& type, const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }

    if (type == "autocancel")
        parent->deleteAutoCancel();
    else if (type == "autorestore")
        parent->deleteAutoRestore();
    else if (type == "autoarchive")
        parent->deleteAutoArchive();

    auto client = get_client(request);
    client->replace(path, defs->print(), false, false);
}

template <typename T>
void update_attribute_in_path(const T& attr, const std::string& type, const httplib::Request& request) {
    const std::string path = request.matches[1];

    auto defs   = std::make_shared<Defs>(*get_defs());
    auto parent = defs->findAbsNode(path);

    if (parent.get() == nullptr) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "Path " + path + " not found");
    }

    if (type == "autocancel")
        parent->deleteAutoCancel();
    else if (type == "autorestore")
        parent->deleteAutoRestore();
    else if (type == "autoarchive")
        parent->deleteAutoArchive();

    add_attribute_to_node(parent, attr);

    auto client = get_client(request);
    client->replace(path, defs->print(), false, false);
}

template <typename T>
T create_from_text(const std::string& line);

template <>
ecf::AutoArchiveAttr create_from_text(const std::string& line) {
    if (line.find_first_of(':') == std::string::npos) {
        try {
            return ecf::AutoArchiveAttr(std::stoi(line));
        }
        catch (const std::exception& e) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid value for autoarchive: " + line + " (" + e.what() + ")");
        }
    }
    else {
        int hour      = 0;
        int min       = 0;
        bool relative = ecf::TimeSeries::getTime(line, hour, min);
        return ecf::AutoArchiveAttr(ecf::TimeSlot(hour, min), relative);
    }
}
template <>
ecf::AutoCancelAttr create_from_text(const std::string& line) {
    if (line.find_first_of(':') == std::string::npos) {
        try {
            return ecf::AutoCancelAttr(std::stoi(line));
        }
        catch (const std::exception& e) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid value for autocancel: " + line + " (" + e.what() + ")");
        }
    }
    else {
        int hour      = 0;
        int min       = 0;
        bool relative = ecf::TimeSeries::getTime(line, hour, min);
        return ecf::AutoCancelAttr(ecf::TimeSlot(hour, min), relative);
    }
}
template <>
ecf::AutoRestoreAttr create_from_text(const std::string& line) {
    std::vector<std::string> tasks;
    ecf::Str::split(line, tasks, " ");

    return ecf::AutoRestoreAttr(tasks);
}

ojson add_node_attribute(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string type  = payload.at("type");
    const std::string value = json_type_to_string(payload.at("value"));

    auto client = get_client(request);

    if (type == "meter") {
        const std::string name = payload.at("name");
        int min                = stoi(json_type_to_string(payload.at("min")));
        int max                = stoi(json_type_to_string(payload.at("max")));
        const Meter m(name, min, max, std::numeric_limits<int>::max(), stoi(value));
        add_attribute_to_path(m, request);
        // Adding a meter always results to having its value == min, even if the value
        // of argument 'value' is something different
        client->alter(path, "change", "meter", name, value);
    }
    else if (type == "event") {
        const std::string name = payload.at("name");
        const Event e(std::numeric_limits<int>::max(), name, (value == "true") ? true : false);
        add_attribute_to_path(e, request);
        if (value != "set" && value != "true" && value != "clear" && value != "false") {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "'value' for event must be one of: set/true, clear/false");
        }
        std::string value_ = (value == "true" || value == "set") ? "set" : "clear";
        client->alter(path, "change", "event", name, value_);
    }
    else if (type == "cron") {
        add_attribute_to_path(ecf::CronAttr::create(value), request);
    }
    else if (type == "complete") {
        add_attribute_to_path(value, request);
    }
    else if (type == "autocancel") {
        add_attribute_to_path(create_from_text<ecf::AutoCancelAttr>(value), request);
    }
    else if (type == "autorestore") {
        add_attribute_to_path(create_from_text<ecf::AutoRestoreAttr>(value), request);
    }
    else if (type == "autoarchive") {
        add_attribute_to_path(create_from_text<ecf::AutoArchiveAttr>(value), request);
    }
    else if (type == "time" || type == "day" || type == "date" || type == "today" || type == "late") {
        client->alter(path, "add", type, value);
    }
    else {
        const std::string name = payload.at("name");
        client->alter(path, "add", type, name, value);
    }

    ojson j;
    j["path"]    = path;
    j["message"] = "Attribute added successfully";
    return j;
}

ojson update_node_attribute(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    std::string type = payload.at("type");

    // Updating node attributes is not a straightforward matter.
    // Firstly, it can be done either as a child command or a user
    // command. We deal with both.
    // Secondly, when it comes to user commands, the current client
    // framework is not nearly covering all cases (attributes):
    // * Some attributes can be added,changed and delete with ClientInvoker
    // * Some attributes can be either added, changed or deleted with ClientInvoker
    // * Some attributes cannot be modified in any way with ClientInvoker
    //
    // Because I want the API to be consistent, it will bend over backwards
    // to allow user to modify all ecFlow attributes, whether the Client code
    // supports it or not.

    struct Ancillary
    {
        std::string type;
        std::string action;
        std::string value;
    };
    std::optional<Ancillary> ancillary;

    if (payload.contains("ECF_NAME")) {
        const std::string name = payload.at("name");

        // this is a child command call
        if (ecf::Child::valid_child_cmd(type) == false) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid action for child command: " + name);
        }

        auto client = get_client_for_tasks(request, payload);

        if (type == "event") {
            const std::string value = payload.at("value");
            if (value != "set" && value != "true" && value != "clear" && value != "false") {
                throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                          "'value' for event must be one of: set/true, clear/false");
            }
            client->child_event(name, (value == "true" || value == "set"));
        }
        else if (type == "meter") {
            const std::string value = payload.at("value");
            client->child_meter(name, std::stoi(value));
        }
        else if (type == "label") {
            const std::string value = payload.at("value");
            client->child_label(name, value);
        }
        else if (type == "queue") {
            auto queue_action = payload.at("queue_action").get<std::string>();
            auto queue_step   = payload.contains("queue_step") ? payload.at("queue_step").get<std::string>() : "";
            auto queue_path   = payload.contains("queue_path") ? payload.at("queue_path").get<std::string>() : "";
            std::string reply = client->child_queue(name, queue_action, queue_step, queue_path);

            if (queue_action == "active" || queue_action == "no_of_aborted") {
                ancillary = std::make_optional(Ancillary{type, queue_action, reply});
            }
        }
        else {
            throw HttpServerException(HttpStatusCode::server_error_not_implemented,
                                      "Child action " + type + ", on attribute " + name + ", is not supported");
        }
    }
    else {
        auto client = get_client(request);

        if (type == "repeat" || type == "late" || type == "complete") {
            const std::string value = json_type_to_string(payload.at("value"));
            client->alter(path, "change", type, value);
        }
        else if (type == "today" || type == "time" || type == "late") {
            const std::string value     = json_type_to_string(payload.at("value"));
            const std::string old_value = json_type_to_string(payload.at("old_value"));
            client->alter(path, "change", type, old_value, value);
        }
        else if (type == "day" || type == "date") {
            const std::string value     = json_type_to_string(payload.at("value"));
            const std::string old_value = json_type_to_string(payload.at("old_value"));
            client->alter(path, "delete", type, old_value);
            client->alter(path, "add", type, value);
        }
        else if (type == "autocancel") {
            const std::string value = json_type_to_string(payload.at("value"));
            update_attribute_in_path(create_from_text<ecf::AutoCancelAttr>(value), type, request);
            trigger_defs_update();
        }
        else if (type == "autorestore") {
            const std::string value = json_type_to_string(payload.at("value"));
            update_attribute_in_path(create_from_text<ecf::AutoRestoreAttr>(value), type, request);
            trigger_defs_update();
        }
        else if (type == "autoarchive") {
            const std::string value = json_type_to_string(payload.at("value"));
            update_attribute_in_path(create_from_text<ecf::AutoArchiveAttr>(value), type, request);
            trigger_defs_update();
        }
        else if (type == "cron") {
            const std::string value     = json_type_to_string(payload.at("value"));
            const std::string old_value = json_type_to_string(payload.at("old_value"));
            client->alter(path, "delete", type, old_value);
            trigger_defs_update([&] { add_attribute_to_path(ecf::CronAttr::create(value), request); });
        }
        else if (type == "limit") {
            const std::string name = payload.at("name");
            const static std::vector<std::string> keys{"max", "value"};
            std::string value;
            try {
                value = json_type_to_string(payload.at("value"));
                type  = type + "_value";
            }
            catch (const ojson::out_of_range& e) {
                try {
                    value = json_type_to_string(payload.at("max"));
                    type  = type + "_max";
                }
                catch (const ojson::out_of_range& e) {
                    throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                              "For limit either 'max' or 'value' must be defined");
                }
            }
            client->alter(path, "change", type, name, value);
        }
        else {
            const std::string name = payload.at("name");
            std::string value      = json_type_to_string(payload.at("value"));
            if (type == "event") {
                if (value != "set" && value != "true" && value != "clear" && value != "false") {
                    throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                              "'value' for event must be one of: set/true, clear/false");
                }
                value = (value == "true" || value == "set") ? "set" : "clear";
            }
            client->alter(path, "change", type, name, value);
        }
    }

    // Package the response body
    ojson j;
    j["path"]    = path;
    j["message"] = "Attribute changed successfully";
    if (ancillary) {
        if (ancillary->action == "active") {
            j["step"] = ancillary->value;
        }
        else if (ancillary->action == "no_of_aborted") {
            j["no_of_aborted"] = ancillary->value;
        }
    }
    return j;
}

ojson delete_node_attribute(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string type = payload.at("type");
    auto client            = get_client(request);

    if (type == "time" || type == "today" || type == "day" || type == "date" || type == "cron" || type == "late" ||
        type == "complete") {
        const std::string value = payload.at("value");
        client->alter(path, "delete", type, value);
    }
    else if (type == "autocancel" || type == "autorestore" || type == "autoarchive") {
        remove_attribute_from_path(type, request);
    }
    else {
        const std::string name = payload.at("name");
        client->alter(path, "delete", type, name);
    }

    ojson j;
    j["path"]    = path;
    j["message"] = "Attribute deleted successfully";
    return j;
}

ojson add_server_attribute(const httplib::Request& request) {
    const ojson payload    = ojson::parse(request.body);
    const std::string type = payload.at("type");

    if (type != "variable") {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Only variables can be added to server attributes");
    }

    const std::string name  = payload.at("name");
    const std::string value = json_type_to_string(payload.at("value"));

    auto client = get_client(request);
    client->alter("/", "add", type, name, value);

    ojson j;
    j["path"]    = "/";
    j["message"] = "Attribute added successfully";
    return j;
}

ojson update_server_attribute(const httplib::Request& request) {
    const ojson payload     = ojson::parse(request.body);
    const std::string type  = payload.at("type");
    const std::string name  = payload.at("name");
    const std::string value = payload.at("value");
    std::string x;

    if (get_defs()->server().find_user_variable(name, x) == false) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "User variable not found");
    }
    else if (type != "variable") {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Only variables can be updated in server attributes");
    }

    auto client = get_client(request);
    client->alter("/", "change", type, name, value);

    ojson j;
    j["path"]    = "/";
    j["message"] = "Attribute changed successfully";
    return j;
}

ojson delete_server_attribute(const httplib::Request& request) {

    const ojson payload = ojson::parse(request.body);

    const std::string type = payload.at("type");
    const std::string name = payload.at("name");
    std::string x;

    if (get_defs()->server().find_user_variable(name, x) == false) {
        throw HttpServerException(HttpStatusCode::client_error_not_found, "User variable not found");
    }
    else if (type != "variable") {
        throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                  "Only variables can be removed from server attributes");
    }

    auto client = get_client(request);
    client->alter("/", "delete", type, name);

    ojson j;
    j["path"]    = "/";
    j["message"] = "Attribute deleted successfully";
    return j;
}

ojson update_node_status(const httplib::Request& request) {
    const std::string path = request.matches[1];
    const ojson payload    = ojson::parse(request.body);

    const std::string name = payload.at("action");

    if (payload.contains("ECF_NAME")) {
        // this is a child command call
        if (ecf::Child::valid_child_cmd(name) == false) {
            throw HttpServerException(HttpStatusCode::client_error_bad_request,
                                      "Invalid action for child command: " + name);
        }

        auto client = get_client_for_tasks(request, payload);

        if (name == "init") {
            client->child_init();
        }
        else if (name == "abort") {
            client->child_abort(payload.value("abort_why", ""));
        }
        else if (name == "complete") {
            client->child_complete();
        }
        else if (name == "wait") {
            client->child_wait(payload.at("wait_expression"));
        }
        else {
            throw HttpServerException(HttpStatusCode::server_error_not_implemented,
                                      "Child action " + name + " not supported");
        }
    }
    else {
        auto client = get_client(request);

        bool recursive = payload.value("recursive", false);
        bool force     = payload.value("force", false);

        if (name == "begin") {
            client->begin(path);
        }
        else if (name == "resume") {
            client->resume(path);
        }
        else if (name == "requeue") {
            client->requeue(path);
        }
        else if (name == "suspend") {
            client->suspend(path);
        }
        else if (name == "defstatus") {
            client->alter(path, "change", name, payload.at("defstatus_value").get<std::string>());
        }
        else if (name == "execute") {
            client->run(path, true);
        }
        else if (name == "archive") {
            client->archive(path, force);
        }
        else if (name == "restore") {
            client->restore(path);
        }
        else {
            std::string name_ = name;
            if (name_ == "abort") {
                name_ = "aborted";
            }
            else if (name_ == "rerun") {
                name_ = "queued";
            }
            else if (name_ == "execute") {
                name_ = "run";
            }
            else if (name_ == "submit") {
                name_ = "submitted";
            }

            client->force(path, name_, recursive);
        }
    }
    ojson j;
    j["path"]    = path;
    j["message"] = "Status changed successfully";
    return j;
}

#if 0
json update_script_content(const httplib::Request& request) {
   const std::string path = request.matches[1];
   const json payload = json::parse(request.body);

   const std::string script = payload.at("script");

   std::vector<std::string> lines;
   lines.reserve(20);
   std::stringstream ss(script);

   for (std::string line; std::getline(ss, line, '\n');) {
      lines.push_back(line);
   }

   auto client = get_client(request);
   client->edit_script_submit(path, {}, lines, false, false);

   json j;

   j["message"] = "Script submitted successfully";
   return j;
}
#endif

void update_defs_loop(int interval) {
    std::thread t([&]() {
        ClientInvoker client;
        //      client.set_auto_sync(true);

        auto get_current_time = [] {
            struct timeval curtime;
            gettimeofday(&curtime, nullptr);
            return static_cast<unsigned int>(curtime.tv_sec);
        };

        auto update = [&] {
            {
                std::lock_guard<std::mutex> lock(def_mutex);
                if (defs_ != nullptr)
                    client.sync(defs_);
                defs_ = client.defs();
            }
            if (opts.verbose) {
                printf("Defs modify_change_no: %d state_change_no: %d\n",
                       defs_->modify_change_no(),
                       defs_->state_change_no());
            }
        };

        // These will throw is ecflow server is not present; we will not
        // try to reconnect if there wasn't a connection to begin with
        client.news_local();
        client.sync_local();

        // Implement a drift to update cycle. The basic idea is that if we
        // don't get requests to the interface, we slowly start to increase
        // the interval which we use to poll ecFlow server. This is done to
        // reduce ecFlow server load when there is no activity on the REST
        // API.
        // For every minute that goes by without any activity on the REST API,
        // we increase the drift by one second. The maximum drift value is
        // 10 * polling interval, but minimum 30 seconds. Activity on the API
        // will reset drift to zero.

        const std::chrono::seconds base_sleeptime(interval);
        std::chrono::seconds sleeptime(interval);
        const std::chrono::seconds max_sleeptime(std::max(30, opts.max_polling_interval));
        std::chrono::seconds previous_sleeptime(interval);

        for (;;) {
            try {
                while (true) {
                    std::unique_lock<std::mutex> lock(cv_mutex);
                    if (defs_cv.wait_for(lock, sleeptime, [] { return update_defs.load(); })) {
                        // update requested by some other thread
                        if (opts.verbose)
                            printf("defs update requested\n");
                        update();
                        update_defs = false;
                    }
                    else {
                        // update triggered by timeout
                        client.news(defs_);
                        if (client.get_news()) {
                            update();
                            update_defs = false;
                        }
                    }

                    if (opts.max_polling_interval <= opts.polling_interval) {
                        // drift disabled
                        continue;
                    }
                    const double last_request_age = static_cast<double>(get_current_time() - last_request_time.load());
                    const auto drift = std::chrono::seconds(static_cast<int>(floor(last_request_age / 60.)));

                    sleeptime = min(max_sleeptime, base_sleeptime + drift);
                    if (opts.verbose && sleeptime != previous_sleeptime) {
                        print_polling_interval_notification(
                            sleeptime.count(), base_sleeptime.count(), drift.count(), max_sleeptime.count());
                    }

                    previous_sleeptime = sleeptime;
                }
            }
            catch (const std::exception& e) {
                printf("ERROR: Communication problem with ecflow server? Retrying in 5s\n");
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    });

    t.detach();

    while (update_defs.load() == true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace ecf::http
