#include <iostream>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QUrl>

#include "connector.hpp"

connector::connector(QObject *parent):QObject(parent)
{
    connect(&m_manager, &QNetworkAccessManager::finished, this, &connector::onResult);
}

void connector::connected(const QString& string,quint16 port)
{
    QUrl url(string);
    url.setPort(port);
    m_manager.get(QNetworkRequest(url));
}


void connector::onResult(QNetworkReply *reply)
{
    if(!reply->error()){
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit jSonDoc(doc);
    }
}

connector::~connector()
{
}
