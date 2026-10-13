import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic


ApplicationWindow {
    id: window
    width: 640
    height: 480
    minimumWidth: 200
    minimumHeight: 250
    visible: true
    title: qsTr("Dash Board")
    property bool lightMode: Application.styleHints.colorScheme === Qt.Light
    property color reallyDark: "#1f1f1f"
    property color dark: "#262626"
    property color reallyLight: "#e7e7e7"
    property color light: "#e0e0e0"

    TemperatureClient {
        id: tempClient
        hostname: hostField.text
        port: parseInt(portField.text)
        topic: topicField.text
        Component.onCompleted: connectToBroker()
    }

    Rectangle {
        id: rectangle1
        anchors.fill: parent
        color: window.lightMode ? window.reallyLight : window.reallyDark

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                TextField { id: hostField; text: "localhost"; placeholderText: qsTr("Broker"); Layout.fillWidth: true }
                TextField { id: portField; text: "1883"; implicitWidth: 70; validator: IntValidator { bottom: 1; top: 65535 } }
                Button {
                    text: tempClient.connected ? qsTr("Disconnect") : qsTr("Connect")
                    onClicked: tempClient.connected ? tempClient.disconnectFromBroker() : tempClient.connectToBroker()
                }
            }

            TextField { id: topicField; text: "sensors/temperature"; placeholderText: qsTr("Topic"); Layout.fillWidth: true }

            Item { Layout.fillHeight: true }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Temperature")
                font.pixelSize: 20
                color: window.lightMode ? window.dark : window.light
            }

            Label {
                Layout.alignment: Qt.AlignHCenter
                text: tempClient.hasValue ? tempClient.temperature.toFixed(1) + " °C" : "--.- °C"
                font.pixelSize: 64
                font.bold: true
                color: window.lightMode ? window.reallyDark : window.reallyLight
            }

            Item { Layout.fillHeight: true }

            Label {
                text: tempClient.status
                color: tempClient.connected ? "#3fa34d" : "#c0392b"
            }
        }
    }
}

        