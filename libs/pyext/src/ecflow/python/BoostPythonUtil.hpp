/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_python_BoostPythonUtil_HPP
#define ecflow_python_BoostPythonUtil_HPP

#include <vector>

#include <boost/python.hpp> // IWYU pragma: keep

class Variable;

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects

class BoostPythonUtil {
public:
    BoostPythonUtil()                                  = delete;
    BoostPythonUtil(const BoostPythonUtil&)            = delete;
    BoostPythonUtil& operator=(const BoostPythonUtil&) = delete;

    /// Convert python list to a vector of integers. raises a type error if integer extraction fails
    static void list_to_int_vec(const boost::python::list& list, std::vector<int>& int_vec);
    static void list_to_str_vec(const boost::python::list& list, std::vector<std::string>& int_vec);
    static void list_to_str_vec(const boost::python::list& list, std::vector<Variable>& vec);
    static void dict_to_str_vec(const boost::python::dict& dict,
                                std::vector<std::pair<std::string, std::string>>& str_pair);
    static void dict_to_str_vec(const boost::python::dict& dict, std::vector<Variable>& vec);
};

template <typename T>
const T copyObject(const T& v) {
    return v;
}

#endif /* ecflow_python_BoostPythonUtil_HPP */
