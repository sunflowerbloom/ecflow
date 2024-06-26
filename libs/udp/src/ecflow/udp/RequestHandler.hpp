/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_udp_RequestHandler_HPP
#define ecflow_udp_RequestHandler_HPP

#include <string>

namespace ecf {

/**
 * Enables the handling of all requests by a ecFlow UDP server
 */
struct RequestHandler
{
public:
    using inbound_t = std::string;

public:
    void handle(const inbound_t& request) const;
};

} // namespace ecf

#endif /* ecflow_udp_RequestHandler_HPP */
