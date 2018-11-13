//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef TIMELINEMODEL_CPP
#define TIMELINEMODEL_CPP

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

class TimelineData;

class TimelineModel : public QAbstractItemModel
{
public:
    explicit TimelineModel(QObject *parent=0);
    ~TimelineModel();

    int columnCount (const QModelIndex& parent = QModelIndex() ) const;
    int rowCount (const QModelIndex& parent = QModelIndex() ) const;

    Qt::ItemFlags flags ( const QModelIndex & index) const;
    QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
    QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

    QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent (const QModelIndex & ) const;

    void setData(TimelineData* data);

    TimelineData* data() const {return data_;}
    void clearData();
    bool hasData() const;

protected:
    TimelineData* data_;
};

class TimelineSortModel : public QSortFilterProxyModel
{
public:
    TimelineSortModel(TimelineModel*,QObject *parent=0);
    ~TimelineSortModel();

    //From QSortFilterProxyModel:
    //we set the source model in the constructor. So this function should not do anything.
    void setSourceModel(QAbstractItemModel*) {}
    TimelineModel* tlModel() const {return tlModel_;}

    void selectionChanged(QModelIndexList lst);
    void setSkipSort(bool b) {skipSort_=b;}
    void setPathFilter(QString);
    void setTaskFilter(bool);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    bool filterAcceptsRow(int sourceRow, const QModelIndex &/*sourceParent*/) const;

    TimelineModel* tlModel_;
    bool skipSort_;
    QString pathFilter_;
    bool taskFilter_;
};


#endif // TIMELINEMODEL_CPP
