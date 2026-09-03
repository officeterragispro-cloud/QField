import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
  id: root
  property string title
  property string subtitle
  property var entries: []
  signal actionRequested(string action)
  clip: true
  contentWidth: availableWidth

  ColumnLayout {
    width: parent.width - 60
    x: 30
    y: 30
    spacing: 20
    Label { text: root.title; color: "#0A2B50"; font.pixelSize: 28; font.bold: true }
    Label { text: root.subtitle; color: "#527086"; font.pixelSize: 14; wrapMode: Text.WordWrap; Layout.fillWidth: true }
    GridLayout {
      Layout.fillWidth: true
      columns: width > 820 ? 3 : 2
      columnSpacing: 14
      rowSpacing: 14
      Repeater {
        model: root.entries
        delegate: Rectangle {
          required property var modelData
          Layout.fillWidth: true
          Layout.preferredHeight: 168
          radius: 14
          color: "white"
          border.color: featureMouse.containsMouse ? "#2A6497" : "#D4E1ED"
          ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 7
            Rectangle { Layout.preferredWidth: tagLabel.width + 16; Layout.preferredHeight: 23; radius: 11; color: "#EBF3FD"; Label { id: tagLabel; anchors.centerIn: parent; text: modelData.tag; color: "#2A6497"; font.pixelSize: 9; font.bold: true } }
            Label { text: modelData.title; color: "#0A2B50"; font.pixelSize: 17; font.bold: true }
            Label { Layout.fillWidth: true; Layout.fillHeight: true; text: modelData.text; color: "#527086"; font.pixelSize: 12; wrapMode: Text.WordWrap }
            Label { text: qsTr("Open  →"); color: "#145A1C"; font.pixelSize: 12; font.bold: true }
          }
          MouseArea { id: featureMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.actionRequested(parent.modelData.action) }
        }
      }
    }
  }
}
