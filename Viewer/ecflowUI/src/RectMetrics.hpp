/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_RectMetrics_HPP
#define ecflow_viewer_RectMetrics_HPP

class RectMetrics {
public:
    explicit RectMetrics(int penWidth);
    int topOffset() const { return topOffset_; }
    int bottomOffset() const { return bottomOffset_; }

protected:
    int topOffset_;
    int bottomOffset_;

private:
    void compute(int);
};

#endif /* ecflow_viewer_RectMetrics_HPP */
