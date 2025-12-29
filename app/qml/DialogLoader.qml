
import QtQuick

Loader {
  id: root
  active: false
  asynchronous: true

  property var dialog: null

  function activate() {
    if (!active)
      active = true

    else if (dialog) {
      dialog.raise()
      dialog.requestActivate()
    }
  }

  onLoaded: {
    root.dialog = item
    dialog.show()
    dialog.onClosing.connect(function() {
      root.active = false;
    })
  }
}
