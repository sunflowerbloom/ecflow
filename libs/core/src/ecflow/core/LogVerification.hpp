/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_core_LogVerification_HPP
#define ecflow_core_LogVerification_HPP

#include <string>
#include <vector>

namespace ecf {

class LogVerification {
public:
    // Disable default construction
    LogVerification() = delete;
    // Disable copy (and move) semantics
    LogVerification(const LogVerification&)                  = delete;
    const LogVerification& operator=(const LogVerification&) = delete;

    /// Given a log file, extract in order. The node_path and the state
    static bool extractNodePathAndState(const std::string& logfile,
                                        std::vector<std::pair<std::string, std::string>>& pathStateVec,
                                        std::string& errorMsg);

    /// Will compare the input log file, with gold reference.
    /// Will compare the node state changes only
    /// Compensate for states that are scheduler dependent
    static bool
    compareNodeStates(const std::string& logfile, const std::string& goldenRefLogFile, std::string& errorMsg);
};

} // namespace ecf

#endif /* ecflow_core_LogVerification_HPP */
