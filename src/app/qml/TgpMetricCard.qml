import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
  property string title
  property string value
  property color accent
  implicitHeight: 104
  radius: 14
  color: "white"
  border.color: "#D4E1ED"

  RowLayout {
    anchors.fill: parent
    anchors.margins: 18
    Rectangle {
      Layout.preferredWidth: 8
      Layout.preferredHeight: 46
      radius: 4
      color: parent.parent.accent
    }
    ColumnLayout {
      spacing: 2
      Label {
        text: parent.parent.parent.title
        color: "#527086"
        font.pixelSize: 12
      }
      Label {
        text: parent.parent.parent.value
        color: "#0A2B50"
        font.pixelSize: 27
        font.bold: true
      }
    }
    Item {
      Layout.fillWidth: true
    }
  }
}
