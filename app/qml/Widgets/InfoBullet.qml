

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
  id: root
  spacing: 8
  Layout.maximumWidth: parent ? parent.width : implicitWidth

  property alias text: label.text
  property alias bulletText: bullet.text
  property alias bulletColor: bullet.color

  Label {
    id: bullet
    text: "✔"
    color: "#27AE60"
    font: Cpp_Misc_CommonFonts.customUiFont(1.33, true)
  }

  Label {
    id: label
    Layout.fillWidth: true
    wrapMode: Label.WordWrap
  }
}
