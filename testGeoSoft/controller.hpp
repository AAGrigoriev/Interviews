#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <QObject>
#include <connector.hpp>
#include <sockets.hpp>
#include <jsonModel.hpp>
#include <QString>

class controller : public QObject
{
    Q_OBJECT

public:
    explicit controller(QObject* parent = nullptr);

    void setConnection(const QString& connection,quint16 port);

    jsonModel* getModel();

private:

   // connector m_connector;
    sockets   m_socket;

    jsonModel m_model;
};



#endif // CONTROLLER_HPP
