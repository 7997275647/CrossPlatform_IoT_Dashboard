import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

ApplicationWindow {
    id: window
    width: 1100
    height: 820
    minimumWidth: 480
    minimumHeight: 480
    visible: true
    title: qsTr("IoT Sensor Dashboard")

    property bool lightMode: Application.styleHints.colorScheme === Qt.Light
    readonly property color bgColor: lightMode ? "#e7e7e7" : "#1f1f1f"
    readonly property color cardColor: lightMode ? "#f6f6f6" : "#2a2a2a"
    readonly property color fgColor: lightMode ? "#1f1f1f" : "#e7e7e7"
    readonly property color mutedColor: lightMode ? "#5a5a5a" : "#a8a8a8"
    readonly property color lineColor: lightMode ? "#20000000" : "#20ffffff"
    readonly property var seriesColors: ["#e4572e", "#29b36b", "#3a86ff"]

    function formatValues(channel) {
        if (!channel.hasValue)
            return "--"
        const v = channel.latest
        if (v.length === 1)
            return v[0].toFixed(1) + " " + channel.unit
        return v.map((x, i) => channel.seriesNames[i] + " " + x.toFixed(1)).join("  ")
    }

    SensorHub {
        id: hub
        hostname: hostField.text
        port: parseInt(portField.text)
        baseTopic: topicField.text
        Component.onCompleted: connectToBroker()
    }

    Rectangle {
        anchors.fill: parent
        color: window.bgColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Connection bar
        Flow {
            Layout.fillWidth: true
            spacing: 8
            TextField { id: hostField; text: "localhost"; placeholderText: qsTr("Broker"); width: 180 }
            TextField {
                id: portField
                text: "1883"
                width: 80
                validator: IntValidator { bottom: 1; top: 65535 }
            }
            TextField { id: topicField; text: "sensors"; placeholderText: qsTr("Base topic"); width: 160 }
            Button {
                text: hub.connected ? qsTr("Disconnect") : qsTr("Connect")
                onClicked: hub.connected ? hub.disconnectFromBroker() : hub.connectToBroker()
            }
            Label {
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: hub.status
                color: hub.connected ? "#3fa34d" : "#c0392b"
            }
        }

        // Latest value tiles
        GridLayout {
            Layout.fillWidth: true
            columns: window.width < 700 ? 1 : 3
            columnSpacing: 12
            rowSpacing: 12

            Repeater {
                model: hub.channels
                delegate: Rectangle {
                    id: tile
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 96
                    radius: 8
                    color: window.cardColor

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 2
                        Label {
                            text: tile.modelData.title
                            color: window.mutedColor
                            font.pixelSize: 14
                        }
                        Label {
                            Layout.fillWidth: true
                            text: window.formatValues(tile.modelData)
                            color: window.fgColor
                            font.pixelSize: tile.modelData.seriesNames.length > 1 ? 22 : 34
                            font.bold: true
                            fontSizeMode: Text.Fit
                            minimumPixelSize: 12
                        }
                        Label {
                            text: tile.modelData.hasValue
                                  ? qsTr("%1 · %2/%3 samples").arg(Qt.formatTime(tile.modelData.lastUpdate, "hh:mm:ss"))
                                        .arg(tile.modelData.count).arg(tile.modelData.capacity)
                                  : tile.modelData.topic
                            color: window.mutedColor
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        // View controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            TabBar {
                id: viewTabs
                TabButton { text: qsTr("Graphs") }
                TabButton { text: qsTr("Tables") }
            }
            Item { Layout.fillWidth: true }
            Label { text: qsTr("Samples"); color: window.mutedColor }
            ComboBox {
                id: windowBox
                model: [64, 256, 512, 1024]
                currentIndex: 3
            }
            CheckBox {
                id: statsBox
                text: qsTr("Min/Max/Avg")
                palette.windowText: window.fgColor
            }
            Button {
                text: qsTr("Export CSV")
                onClicked: {
                    const paths = []
                    for (const ch of hub.channels) {
                        const p = ch.exportCsv()
                        if (p) paths.push(p)
                    }
                    exportLabel.text = paths.length > 0 ? qsTr("Saved: %1").arg(paths.join(", ")) : qsTr("Export failed")
                }
            }
            Button {
                text: qsTr("Clear")
                onClicked: for (const ch of hub.channels) ch.clear()
            }
        }

        Label {
            id: exportLabel
            Layout.fillWidth: true
            visible: text.length > 0
            color: window.mutedColor
            elide: Text.ElideMiddle
            font.pixelSize: 11
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: viewTabs.currentIndex

            // Graphs: one card per sensor, stacked
            ColumnLayout {
                spacing: 12
                Repeater {
                    model: hub.channels
                    delegate: Rectangle {
                        id: card
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 100
                        radius: 8
                        color: window.cardColor

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 4
                            RowLayout {
                                Label {
                                    text: card.modelData.title + " (" + card.modelData.unit + ")"
                                    color: window.fgColor
                                    font.bold: true
                                }
                                Item { Layout.fillWidth: true }
                                Repeater {
                                    model: card.modelData.seriesNames
                                    delegate: RowLayout {
                                        id: legend
                                        required property int index
                                        required property string modelData
                                        spacing: 4
                                        Rectangle {
                                            width: 10; height: 10; radius: 5
                                            color: window.seriesColors[legend.index % window.seriesColors.length]
                                        }
                                        Label {
                                            text: legend.modelData
                                            color: window.mutedColor
                                            font.pixelSize: 11
                                        }
                                    }
                                }
                            }
                            SensorChart {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                channel: card.modelData
                                windowSize: parseInt(windowBox.currentText)
                                showStats: statsBox.checked
                                textColor: window.mutedColor
                                gridColor: window.lineColor
                                seriesColors: window.seriesColors
                            }
                        }
                    }
                }
            }

            // Tables: one column per sensor
            RowLayout {
                spacing: 12
                Repeater {
                    model: hub.channels
                    delegate: Rectangle {
                        id: tableCard
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: window.cardColor

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            Label {
                                text: tableCard.modelData.title + " (" + tableCard.modelData.unit + ")"
                                color: window.fgColor
                                font.bold: true
                            }
                            SensorTable {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                channel: tableCard.modelData
                                textColor: window.fgColor
                                lineColor: window.lineColor
                            }
                        }
                    }
                }
            }
        }
    }
}
