/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_http_HttpServer_HPP
#define ecflow_http_HttpServer_HPP

namespace ecf::http {

class HttpServer {
public:
    HttpServer(int argc, char** argv);
    void run() const;
    ~HttpServer() = default;

private:
    void parse_args(int argc, char** argv) const;
};

} // namespace ecf::http

#endif /* ecflow_http_HttpServer_HPP */
