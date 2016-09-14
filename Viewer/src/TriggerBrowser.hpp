//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TRIGGERBROWSER_HPP
#define TRIGGERBROWSER_HPP

#include <QWidget>

#include "ui_TriggerBrowser.h"

#include "VInfo.hpp"

class TriggeredScanner;

class TriggerBrowser : public QWidget, protected Ui::TriggerBrowser
{
public:
    explicit TriggerBrowser(QWidget *parent=0);

    void setScanner(TriggeredScanner* scanner);
    void load(VInfo_ptr,bool);

protected:
    TriggeredScanner* scanner_;
};

#endif // TRIGGERBROWSER_HPP
