import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 1.4

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    TableView {
        anchors.fill: parent

        TableViewColumn {
            role: "name"
            title: "Name"
        }

        TableViewColumn {
            role: "lat"
            title: "Lat"
        }

        model: jsonModel
    }


}
