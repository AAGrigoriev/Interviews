#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QJsonDocument>

class connector: public QObject
{
    Q_OBJECT
public:
    explicit connector(QObject *parent = 0);
    virtual ~connector();

signals:
    void jSonDoc(QJsonDocument const& doc);
public:
    void connected(QString const& ,quint16 port);
private slots:
    void onResult(QNetworkReply *reply);
private:
    QNetworkAccessManager m_manager;
};

#endif // CONNECTOR_H
