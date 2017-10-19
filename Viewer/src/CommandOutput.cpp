//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "CommandOutput.hpp"

#include "CommandOutputDialog.hpp"

CommandOutputHandler* CommandOutputHandler::instance_=0;

//===============================================
//
// CommandOutput
//
//==============================================

CommandOutput::CommandOutput(QString cmd,QString cmdDef,QDateTime runTime) :
    command_(cmd), commandDef_(cmdDef), runTime_(runTime), status_(RunningStatus)
{

}

void CommandOutput::appendOutput(QString txt)
{
    output_+=txt;
}

void CommandOutput::appendError(QString txt)
{
    error_+=txt;
}

QString CommandOutput::statusStr() const
{
    static QString finishedStr("finished");
    static QString failedStr("failed");
    static QString runningStr("running");

    switch(status_)
    {
    case FinishedStatus:
        return finishedStr;
    case FailedStatus:
        return failedStr;
    case RunningStatus:
        return runningStr;
    default:
        return QString();
    }

    return QString();
}

//===============================================
//
// CommandOutputHandler
//
//===============================================

CommandOutputHandler::CommandOutputHandler(QObject* parent) :
    QObject(parent),
    maxNum_(20)
{

}

CommandOutputHandler* CommandOutputHandler::instance()
{
    if(!instance_)
        instance_=new CommandOutputHandler(0);

    return instance_;
}

int CommandOutputHandler::indexOfItem(CommandOutput_ptr item) const
{
    if(!item)
        return -1;

    for(int i=0; i < items_.count(); i++)
    {
        if(item.get() == items_[i].get())
            return i;
    }

    return -1;
}

void CommandOutputHandler::appendOutput(CommandOutput_ptr item,QString txt)
{
    if(item)
    {
        item->appendOutput(txt);
        CommandOutputDialog::make();
        Q_EMIT itemOutputAppend(item,txt);
    }
}

void CommandOutputHandler::appendError(CommandOutput_ptr item,QString txt)
{
    if(item)
    {
        item->appendError(txt);
        CommandOutputDialog::make();
        Q_EMIT itemErrorAppend(item,txt);
    }
}

void CommandOutputHandler::finished(CommandOutput_ptr item)
{
    if(item)
    {
        item->setStatus(CommandOutput::FinishedStatus);
        Q_EMIT itemStatusChanged(item);
    }
}

void CommandOutputHandler::failed(CommandOutput_ptr item)
{
    if(item)
    {
        item->setStatus(CommandOutput::FailedStatus);
        Q_EMIT itemStatusChanged(item);
    }
}

CommandOutput_ptr CommandOutputHandler::addItem(QString cmd,QString cmdDef,QDateTime runTime)
{
    CommandOutputDialog::make();
    CommandOutput_ptr item=
            CommandOutput_ptr(new CommandOutput(cmd,cmdDef,runTime));
    Q_EMIT itemAddBegin();
    items_ << item;
    Q_EMIT itemAddEnd();
    return item;
}

