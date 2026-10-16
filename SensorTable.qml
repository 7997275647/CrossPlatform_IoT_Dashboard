import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

// Newest-first table of the samples in a SensorChannel.
ColumnLayout {
    id: root

    required property var channel
    property color textColor: "#808080"
    property color lineColor: "#40808080"

    spacing: 0

    Row {
        Layout.fillWidth: true
        Repeater {
            model: root.channel.columnLabels
            Label {
                required property string modelData
                width: table.width / root.channel.columnLabels.length
                text: modelData
                font.bold: true
                color: root.textColor
                padding: 4
            }
        }
    }

    TableView {
        id: table
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: root.channel
        boundsBehavior: Flickable.StopAtBounds
        columnWidthProvider: () => width / root.channel.columnLabels.length
        onWidthChanged: forceLayout()
        ScrollBar.vertical: ScrollBar {}

        delegate: Rectangle {
            required property string display
            required property int row
            implicitHeight: 24
            color: row % 2 === 0 ? "transparent" : root.lineColor
            Label {
                anchors.fill: parent
                anchors.leftMargin: 4
                verticalAlignment: Text.AlignVCenter
                text: parent.display
                color: root.textColor
                elide: Text.ElideRight
            }
        }
    }
}
