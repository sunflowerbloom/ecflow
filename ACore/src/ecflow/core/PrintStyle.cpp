/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include "ecflow/core/PrintStyle.hpp"

#include "ecflow/core/Enumerate.hpp"

namespace ecf::detail {

template <>
struct EnumTraits<PrintStyle::Type_t>
{
    using underlying_t = std::underlying_type_t<PrintStyle::Type_t>;

    static constexpr std::array map = std::array{
        // clang-format off
        std::make_pair(PrintStyle::Type_t::NOTHING, "NOTHING"),
        std::make_pair(PrintStyle::Type_t::DEFS, "DEFS"),
        std::make_pair(PrintStyle::Type_t::STATE, "STATE"),
        std::make_pair(PrintStyle::Type_t::MIGRATE, "MIGRATE"),
        std::make_pair(PrintStyle::Type_t::NET, "NET"),
        // clang-format on
    };
    static constexpr size_t size = map.size();

    static_assert(EnumTraits<PrintStyle::Type_t>::size == map.back().first + 1);
};

} // namespace ecf::detail

static PrintStyle::Type_t style_ = PrintStyle::NOTHING;

PrintStyle::Type_t PrintStyle::getStyle() {
    return style_;
}

void PrintStyle::setStyle(PrintStyle::Type_t f) {
    style_ = f;
}

bool PrintStyle::defsStyle() {
    if (getStyle() == PrintStyle::DEFS || getStyle() == PrintStyle::NOTHING) {
        return true;
    }
    return false;
}

bool PrintStyle::persist_style() {
    if (getStyle() == PrintStyle::MIGRATE || getStyle() == PrintStyle::NET) {
        return true;
    }
    return false;
}

bool PrintStyle::is_persist_style(PrintStyle::Type_t t) {
    if (t == PrintStyle::MIGRATE || t == PrintStyle::NET) {
        return true;
    }
    return false;
}

std::string PrintStyle::to_string() {
    return to_string(getStyle());
}

std::string PrintStyle::to_string(PrintStyle::Type_t t) {
    return std::string{ecf::Enumerate<PrintStyle::Type_t>::to_string(t).value_or("")};
}
