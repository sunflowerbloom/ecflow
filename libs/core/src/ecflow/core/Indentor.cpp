/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/Indentor.hpp"

#include <ostream>

namespace ecf {

int Indentor::index_   = 0;
bool Indentor::indent_ = true;

void Indentor::indent(std::string& os, int char_spaces) {
    os += (indent_ ? std::string(index_ * char_spaces, ' ') : "");
}

} // namespace ecf
