#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>

class sockets : public QObject
{
    Q_OBJECT
public:
    explicit sockets(QObject* parent = nullptr);
    void connectToHost(QString addres,quint16 port);
signals:
    void jSonDoc(QJsonDocument const& doc);

private:

    QTcpSocket m_socket;
    QByteArray m_data;

private slots:

    void startTransfer();
    void readData();
    void displayErrorSlot(QAbstractSocket::SocketError socketError);
    void finishTransfer();


};


#endif // SOCKETS_HPP
