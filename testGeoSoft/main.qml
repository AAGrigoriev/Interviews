import QtQml 2.15
import QtQuick 2.15
import QtQuick.Window 2.3
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("TestGeoTech")
    TableView {
        anchors.fill: parent
        columnSpacing: 1
        rowSpacing: 1
        clip: true

        model: jsonModel

        delegate: Rectangle {
            id: cell
            implicitWidth: 150
            implicitHeight: 50
            border.color: "black"
            border.width: 2
            color: color_warning
            Text {
                id: itext
                text: table_data
                font.pointSize: 12
                anchors.centerIn: parent
            }
        }
    }
}
