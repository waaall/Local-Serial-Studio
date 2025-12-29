

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls

import "../Widgets" as Widgets

Window {
  id: root

  //
  // Window options
  //
  title: Cpp_AppName
  width: minimumWidth
  height: minimumHeight
  minimumWidth: layout.implicitWidth
  maximumWidth: layout.implicitWidth
  minimumHeight: layout.implicitHeight
  maximumHeight: layout.implicitHeight
  Component.onCompleted: {
    root.flags = Qt.Dialog |
        Qt.WindowTitleHint |
        Qt.WindowCloseButtonHint
  }

  //
  // Close shortcut
  //
  Shortcut {
    sequences: [StandardKey.Close]
    onActivated: root.close()
  }

  //
  // Use page item to set application palette
  //
  Page {
    anchors.fill: parent
    palette.mid: Cpp_ThemeManager.colors["mid"]
    palette.dark: Cpp_ThemeManager.colors["dark"]
    palette.text: Cpp_ThemeManager.colors["text"]
    palette.base: Cpp_ThemeManager.colors["base"]
    palette.link: Cpp_ThemeManager.colors["link"]
    palette.light: Cpp_ThemeManager.colors["light"]
    palette.window: Cpp_ThemeManager.colors["window"]
    palette.shadow: Cpp_ThemeManager.colors["shadow"]
    palette.accent: Cpp_ThemeManager.colors["accent"]
    palette.button: Cpp_ThemeManager.colors["button"]
    palette.midlight: Cpp_ThemeManager.colors["midlight"]
    palette.highlight: Cpp_ThemeManager.colors["highlight"]
    palette.windowText: Cpp_ThemeManager.colors["window_text"]
    palette.brightText: Cpp_ThemeManager.colors["bright_text"]
    palette.buttonText: Cpp_ThemeManager.colors["button_text"]
    palette.toolTipBase: Cpp_ThemeManager.colors["tooltip_base"]
    palette.toolTipText: Cpp_ThemeManager.colors["tooltip_text"]
    palette.linkVisited: Cpp_ThemeManager.colors["link_visited"]
    palette.alternateBase: Cpp_ThemeManager.colors["alternate_base"]
    palette.placeholderText: Cpp_ThemeManager.colors["placeholder_text"]
    palette.highlightedText: Cpp_ThemeManager.colors["highlighted_text"]

    ColumnLayout {
      spacing: 0
      id: layout
      anchors.centerIn: parent

      //
      // Window controls
      //
      RowLayout {
        spacing: 0
        Layout.fillWidth: true
        Layout.fillHeight: true

        //
        // NSIS-like banner
        //
        Image {
          id: banner
          Layout.leftMargin: -1
          sourceSize.width: 164
          source: "qrc:/rcc/images/dialog-banner.svg"
        } Rectangle {
          implicitWidth: 1
          Layout.fillHeight: true
          color: Cpp_ThemeManager.colors["groupbox_border"]
        }

        //
        // Spacer
        //
        Item {
          implicitWidth: 24
        }

        //
        // Welcome message
        //
        ColumnLayout {
          spacing: 0
          Layout.fillWidth: true
          Layout.fillHeight: true
          Layout.minimumHeight: banner.implicitHeight
          Layout.maximumHeight: banner.implicitHeight
          Layout.minimumWidth: 400
          Layout.maximumWidth: 400

          Item {
            implicitHeight: 24
          }

          Label {
            Layout.fillWidth: true
            Layout.maximumWidth: parent.width
            text: qsTr("Welcome to %1!").arg(Cpp_AppName)
            font: Cpp_Misc_CommonFonts.customUiFont(1.2, true)
          }

          Item {
            implicitHeight: 16
          }

          Label {
            wrapMode: Label.WordWrap
            Layout.maximumWidth: parent.width
            text: qsTr("Serial Studio is a powerful real-time visualization tool, " +
                       "built for engineers, students, and makers.")
          }

          Item {
            implicitHeight: 12
          }

          Label {
            wrapMode: Label.WordWrap
            Layout.maximumWidth: parent.width
            text: qsTr("This is the open-source GPLv3 version. You are free to use, " +
                       "modify, and redistribute this software under the GPL terms.")
          }

          Item {
            implicitHeight: 12
          }

          Widgets.InfoBullet {
            text: qsTr("Click the \"Help\" toolbar button to get started.")
          }

          Item {
            implicitHeight: 12
          }

          Widgets.InfoBullet {
            text: qsTr("Visit our GitHub repository for documentation and examples.")
          }

          Item {
            Layout.fillHeight: true
          }
        }

        //
        // Another spacer
        //
        Item {
          implicitWidth: 24
        }
      }

      //
      // Buttons
      //
      Rectangle {
        border.width: 1
        Layout.leftMargin: -1
        Layout.rightMargin: -1
        Layout.fillWidth: true
        Layout.bottomMargin: -1
        implicitHeight: buttonsLayout.implicitHeight + 16
        color: Cpp_ThemeManager.colors["groupbox_background"]
        border.color: Cpp_ThemeManager.colors["groupbox_border"]

        RowLayout {
          id: buttonsLayout

          anchors {
            margins: 8
            left: parent.left
            right: parent.right
            verticalCenter: parent.verticalCenter
          }

          Item {
            Layout.fillWidth: true
          }

          Button {
            icon.width: 18
            icon.height: 18
            rightPadding: 8
            text: qsTr("Get Started")
            highlighted: true
            Layout.alignment: Qt.AlignVCenter
            icon.source: "qrc:/rcc/icons/buttons/apply.svg"
            icon.color: Cpp_ThemeManager.colors["button_text"]
            onClicked: {
              app.showMainWindow()
              root.close()
            }
          }
        }
      }
    }
  }
}
