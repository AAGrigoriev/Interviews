#include <controller.hpp>

controller::controller(QObject* parent):QObject(parent) {
    connect(&m_socket,&sockets::jSonDoc,&m_model,&jsonModel::setJsonData);
}

void controller::setConnection(const QString& connection,quint16 port)
{
    m_socket.connectToHost(connection,port);
}

jsonModel* controller::getModel()
{
    return &m_model;
}
