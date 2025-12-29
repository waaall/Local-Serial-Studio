

import QtQuick
import QtQuick.Shapes
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
  required property CompassModel model
  required property var windowRoot

  //
  // Widget layout
  //
  ColumnLayout {
    spacing: 0
    anchors.margins: 8
    anchors.fill: parent

    //
    // Gauge control
    //
    Item {
      id: control

      //
      // Set widget size
      //
      readonly property real widgetSize: Math.min(control.width, control.height)

      //
      // Layout
      //
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.alignment: Qt.AlignHCenter

      //
      // Compass background
      //
      Image {
        anchors.centerIn: parent
        source: "qrc:/rcc/instruments/compass_background.svg"
        sourceSize: Qt.size(control.widgetSize, control.widgetSize)
      }

      //
      // Compass indicator
      //
      Image {
        anchors.centerIn: parent
        rotation: root.model.value
        source: "qrc:/rcc/instruments/compass_needle.svg"
        sourceSize: Qt.size(control.widgetSize, control.widgetSize)
      }

      //
      // Compass case
      //
      Image {
        anchors.centerIn: parent
        source: "qrc:/rcc/instruments/compass_case.svg"
        sourceSize: Qt.size(control.widgetSize, control.widgetSize)
      }
    }

    //
    // Spacer
    //
    Item {
      implicitWidth: 4
    }

    //
    // Range/scale + current value display
    //
    VisualRange {
      id: range
      units: "°"
      alarm: false
      minValue: 0
      maxValue: 360
      rangeVisible: false
      textValue: root.model.text
      maximumWidth: root.width * 0.3
      Layout.alignment: Qt.AlignHCenter
      Layout.minimumWidth: implicitWidth
    }

    //
    // Spacer
    //
    Item {
      implicitWidth: 8
    }
  }
}
