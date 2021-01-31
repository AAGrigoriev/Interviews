#include <controller.hpp>

controller::controller(QObject* parent):QObject(parent),m_connector(this),m_model(this) {

    connect(&m_connector,&connector::jSonDoc,&m_model,&jsonModel::setJsonData);

}

void controller::setConnection(const QString& connection,quint16 port)
{
    m_connector.connected(connection,port);
}

jsonModel* controller::getModel()
{
    return &m_model;
}
