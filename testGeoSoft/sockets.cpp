#include <sockets.hpp>
#include <iostream>

sockets::sockets(QObject* parent):QObject(parent)
{
    connect(&m_socket, SIGNAL(connected()),
            this, SLOT(startTransfer()));
    connect(&m_socket, SIGNAL(readyRead()),
            this, SLOT(readData()));
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayErrorSlot(QAbstractSocket::SocketError)));
    connect(&m_socket, SIGNAL(disconnected()),
            this, SLOT(finishTransfer()));
}

void sockets::connectToHost(QString addres, quint16 port)
{
    m_socket.connectToHost(addres,port);
}

void sockets::startTransfer()
{
    m_socket.write(QByteArray("GET /users HTTP/1.0\nHOST: jsonplaceholder.typicode.com\n\n"));
}

void sockets::readData()
{
    m_data += m_socket.readAll();
}

void sockets::displayErrorSlot(QAbstractSocket::SocketError socket_error)
{
    switch (socket_error) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        break;
    case QAbstractSocket::ConnectionRefusedError:
        break;
    default:
        break;
    }
}


void sockets::finishTransfer()
{
    int indexFind = m_data.indexOf("\r\n\r\n");

    if(indexFind != -1)
    {
        QJsonDocument j_doc = QJsonDocument::fromJson(m_data.mid(indexFind));
        emit jSonDoc(j_doc);
    }
}
