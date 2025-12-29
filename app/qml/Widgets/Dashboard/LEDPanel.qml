

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls

import SerialStudio

import "../"

Item {
  id: root

  //
  // Widget data inputs
  //
  required property color color
  required property var windowRoot
  required property LEDPanelModel model

  //
  // Create scrollable grid view with LED states
  //
  Flickable {
    contentWidth: width
    anchors.fill: parent
    anchors.topMargin: 8
    anchors.leftMargin: 8
    anchors.bottomMargin: 8
    contentHeight: Math.max(grid.implicitHeight, height)

    ScrollBar.vertical: ScrollBar {
      id: scroll
    }

    GridLayout {
      id: grid
      columns: 2
      rowSpacing: 4
      columnSpacing: 4
      width: parent.width - 8
      anchors.centerIn: parent
      anchors.horizontalCenterOffset: -4

      Repeater {
        model: root.model.count
        delegate: Rectangle {
          border.width: 1
          Layout.fillWidth: true
          Layout.minimumHeight: 32
          Layout.maximumHeight: 32
          color: Cpp_ThemeManager.colors["widget_base"]
          border.color: Cpp_ThemeManager.colors["widget_border"]

          RowLayout {
            id: layout
            spacing: 0
            anchors.margins: 4
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter

            Item {
              Layout.fillWidth: true
            }

            Rectangle {
              id: led
              readonly property color ledColor: root.model.colors[index]

              border.width: 1
              implicitWidth: 18
              implicitHeight: 18
              radius: implicitWidth / 2
              Layout.alignment: Qt.AlignVCenter
              border.color: Cpp_ThemeManager.colors["widget_border"]
              color: root.model.states[index] ? ledColor : Qt.darker(ledColor)

              Behavior on color {ColorAnimation{}}
            }

            Item {
              Layout.fillWidth: true
              Layout.minimumWidth: 4
            }

            Label {
              elide: Qt.ElideRight
              text: root.model.titles[index]
              Layout.alignment: Qt.AlignVCenter
              font: Cpp_Misc_CommonFonts.monoFont
              horizontalAlignment: Label.AlignLeft
              Layout.maximumWidth: layout.width - 4 - 24
              color: Cpp_ThemeManager.colors["widget_text"]

              Behavior on color {ColorAnimation{}}
            }

            Item {
              Layout.fillWidth: true
              Layout.minimumWidth: 4
            }
          }

          MultiEffect {
            source: led
            width: led.width
            height: led.height
            x: layout.x + led.x
            y: layout.y + led.y

            blurEnabled: true
            blur: root.model.states[index] ? 1 : 0
            blurMax: root.model.states[index] ? 64 : 0
            brightness: root.model.states[index] ? 0.4 : 0
            saturation: root.model.states[index] ? 0.2 : 0

            enabled: !Cpp_Misc_ModuleManager.softwareRendering
            visible: !Cpp_Misc_ModuleManager.softwareRendering
          }

          MultiEffect {
            source: led
            width: led.width
            height: led.height
            x: layout.x + led.x
            y: layout.y + led.y

            blurEnabled: true
            blur: root.model.states[index] ? 0.3 : 0
            blurMax: root.model.states[index] ? 64 : 0
            brightness: root.model.states[index] ? 0.4 : 0
            saturation: root.model.states[index] ? 0.2 : 0

            enabled: !Cpp_Misc_ModuleManager.softwareRendering
            visible: !Cpp_Misc_ModuleManager.softwareRendering
          }
        }
      }
    }
  }
}
