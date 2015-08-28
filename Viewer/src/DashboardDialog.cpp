//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "DashboardDialog.hpp"

#include "DashboardWidget.hpp"

#include <QCloseEvent>
#include <QSettings>

DashboardDialog::DashboardDialog(QWidget *parent) :
		QDialog(parent),
		dw_(0)
{
	setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);

	readSettings();
}

DashboardDialog::~DashboardDialog()
{
	dw_->deleteLater();
}

void DashboardDialog::add(DashboardWidget* dw)
{
	dw_=dw;

	layout_->insertWidget(0,dw_,1);

    connect(dw_,SIGNAL(titleUpdated(QString)),
            this,SLOT(slotUpdateTitle(QString)));

    dw_->populateDialog();
}

void DashboardDialog::reject()
{
	writeSettings();
	QDialog::reject();
}

void DashboardDialog::closeEvent(QCloseEvent * event)
{
	event->accept();
	writeSettings();
}

void DashboardDialog::slotUpdateTitle(QString txt)
{

    setWindowTitle(txt.remove("<b>").remove("</b>"));
}


void DashboardDialog::writeSettings()
{
	QSettings settings("ECMWF","ecflowUI-DashboardDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	settings.endGroup();
}

void DashboardDialog::readSettings()
{
	QSettings settings("ECMWF","ecflowUI-DashboardDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(520,500));
	}

	settings.endGroup();
}
