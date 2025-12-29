

import QtQuick
import QtQuick.Layouts

import SerialStudio
import "Drivers" as Drivers

Rectangle {
  id: root
  radius: 2
  border.width: 1
  implicitHeight: layout.implicitHeight + 32
  color: Cpp_ThemeManager.colors["groupbox_background"]
  border.color: Cpp_ThemeManager.colors["groupbox_border"]

  //
  // Create list of device panels
  //
  property var buses: []

  //
  // Device configuration
  //
  StackLayout {
    id: layout
    anchors.margins: 8
    anchors.fill: parent
    currentIndex: Cpp_IO_Manager.busType
    implicitHeight: {
      let maxHeight = 0;
      for (let i = 0; i < root.buses.length; ++i) {
        const item = root.buses[i];
        if (item && item.implicitHeight > maxHeight) {
          maxHeight = item.implicitHeight;
        }
      }

      return maxHeight + 32;
    }

    Loader {
      active: true
      asynchronous: true
      Layout.fillWidth: true
      Layout.fillHeight: true
      sourceComponent: Component {
        Drivers.UART {
          Component.onCompleted: root.buses.push(this)
        }
      }
    }

    Loader {
      active: true
      asynchronous: true
      Layout.fillWidth: true
      Layout.fillHeight: true
      sourceComponent: Component {
        Drivers.Network {
          Component.onCompleted: root.buses.push(this)
        }
      }
    }

    Loader {
      active: true
      asynchronous: true
      Layout.fillWidth: true
      Layout.fillHeight: true
      sourceComponent: Component {
        Drivers.BluetoothLE {
          Component.onCompleted: root.buses.push(this)
        }
      }
    }
  }
}
