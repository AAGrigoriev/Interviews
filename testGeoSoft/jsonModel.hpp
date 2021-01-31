#ifndef JSONMODEL_HPP
#define JSONMODEL_HPP
#include <QAbstractTableModel>
#include <qqml.h>
#include <QJsonDocument>
#include <QVector>
#include <DJSon.hpp>

class jsonModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

public slots :

    void setJsonData(QJsonDocument const& doc);

public:

    explicit jsonModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex & = QModelIndex()) const override;

    int columnCount(const QModelIndex & = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

private:
    enum Roles {
        Name = Qt::UserRole + 1,    // Имя
        Lat                         // Долгота
    };

    int m_column_count = 2;
    QVector<jSonParam> m_data;

    void initView();

};


#endif // JSONMODEL_HPP
