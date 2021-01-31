#include <jsonModel.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <utility>
#include <iostream>

jsonModel::jsonModel(QObject *parent):QAbstractTableModel(parent) {}

int jsonModel::rowCount( QModelIndex const & index) const
{
    Q_UNUSED(index)
    return m_data.size();
}

int jsonModel::columnCount( QModelIndex const & index) const
{
    Q_UNUSED(index)
    return  m_column_count;
}

QVariant jsonModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    switch(role)
    {
    case Name:
        return m_data[index.row()].name;
    case Lat:
        return m_data[index.row()].lat;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> jsonModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Name] = "name";
    roles[Lat] =  "lat";
    return roles;
}

void jsonModel::setJsonData(QJsonDocument const & doc)
{
    QJsonArray array = doc.array();

    if(!array.isEmpty())
    {
        for(auto beg = array.begin(); beg != array.end();++beg)
        {
            jSonParam param;
            QJsonObject subtree = beg->toObject();
            param.name = subtree["address"].toObject()["city"].toString();
            param.lat  = subtree["address"].toObject()["geo"].toObject()["lat"].toString().toDouble();
            m_data.push_back(std::move(param));
        }
        initView();
    }
}

void jsonModel::initView()
{
    beginInsertColumns(QModelIndex(),0,m_data.size());
    endInsertRows();

    for(int i = 0 ; i < m_data.size();++i)
    {
        for(int j = 0 ; j < m_column_count; ++j)
        {
            QModelIndex index = createIndex(i,j);
            emit dataChanged(index,index);
        }
    }

}
