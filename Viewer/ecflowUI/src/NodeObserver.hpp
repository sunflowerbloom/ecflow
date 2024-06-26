/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_NodeObserver_HPP
#define ecflow_viewer_NodeObserver_HPP

#include "ecflow/node/Aspect.hpp"
#include "ecflow/node/Node.hpp"

class VNode;
class VNodeChange;

class NodeObserver {
public:
    NodeObserver()          = default;
    virtual ~NodeObserver() = default;

    virtual void
    notifyBeginNodeChange(const VNode* vn, const std::vector<ecf::Aspect::Type>& a, const VNodeChange&)            = 0;
    virtual void notifyEndNodeChange(const VNode* vn, const std::vector<ecf::Aspect::Type>& a, const VNodeChange&) = 0;
};

#endif /* ecflow_viewer_NodeObserver_HPP */
