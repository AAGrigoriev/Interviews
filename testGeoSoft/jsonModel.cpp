#include <jsonModel.hpp>
#include <QJsonArray>
#include <QJsonObject>
#include <utility>
#include <iostream>
#include <QBrush>

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
    if (!index.isValid())
    {
        return QVariant();
    }
    switch(role)
    {
    case Data:
        return m_data.at(index.row()).at(index.column());
    case Color:
        return (index.column() == m_column_count - 1 ) ?
                m_data.at(index.row()).at(index.column()).toDouble() >= 0 ? "green" : "red" : "white";
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> jsonModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Data]  = "table_data";
    roles[Color] = "color_warning";
    return roles;
}

void jsonModel::setJsonData(QJsonDocument const & doc)
{
    QJsonArray array = doc.array();

    if(!array.isEmpty())
    {
        for(auto beg = array.begin(); beg != array.end();++beg)
        {
            QVector<QString> temp;
            QJsonObject subtree = beg->toObject();
            temp.push_back(subtree["address"].toObject()["city"].toString());
            temp.push_back(subtree["address"].toObject()["geo"].toObject()["lat"].toString());
            m_data.push_back(std::move(temp));
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
