

import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Controls

import "Widgets" as Widgets
import "Dialogs" as Dialogs
import "MainWindow" as MainWindow
import "ProjectEditor" as ProjectEditor

Item {
  id: app

  //
  // Define application name
  //
  readonly property bool proVersion: false

  //
  // Check for updates (non-silent mode)
  //
  function checkForUpdates() {
    Cpp_Updater.setNotifyOnFinish(Cpp_AppUpdaterUrl, true)
    Cpp_Updater.checkForUpdates(Cpp_AppUpdaterUrl)
  }

  //
  // Launch welcome dialog or show main window during starup
  //
  Component.onCompleted: {
    app.showMainWindow()
  }

  //
  // Main window + subdialogs
  //
  MainWindow.MainWindow {
    id: mainWindow
    onClosing: (close) => {
                 close.accepted = Cpp_JSON_ProjectModel.askSave()
                 if (close.accepted)
                 Qt.quit()
               }

    Dialogs.Settings {
      id: settingsDialog
    }

    Dialogs.IconPicker {
      id: actionIconPicker
    }

    Dialogs.CsvPlayer {
      id: csvPlayer
    }

    Dialogs.Donate {
      id: donateDialog
    }

    DialogLoader {
      id: aboutDialog
      source: "qrc:/Frequency-Calculator.com/gui/qml/Dialogs/About.qml"
    }

    DialogLoader {
      id: acknowledgementsDialog
      source: "qrc:/Frequency-Calculator.com/gui/qml/Dialogs/Acknowledgements.qml"
    }

    DialogLoader {
      id: fileTransmissionDialog
      source: "qrc:/Frequency-Calculator.com/gui/qml/Dialogs/FileTransmission.qml"
    }
  }

  //
  // Project editor dialog
  //
  ProjectEditor.ProjectEditor {
    id: projectEditor
  }

  // Main Window display function
  //
  function showMainWindow() {
    mainWindow.showWindow()
  }

  //
  // Dialog display functions (FOSS)
  //
  function showAboutDialog()       { aboutDialog.activate() }
  function showSettingsDialog()    { settingsDialog.showNormal() }
  function showProjectEditor()     { projectEditor.displayWindow() }
  function showAcknowledgements()  { acknowledgementsDialog.activate() }
  function showFileTransmission()  { fileTransmissionDialog.activate() }

}
