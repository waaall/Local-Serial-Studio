

#pragma once

#include <QObject>
#include <QMessageBox>
#include <QApplication>

typedef QMap<QMessageBox::StandardButton, QString> ButtonTextMap;

namespace Misc
{
/**
 * @brief The Utilities class
 *
 * The @c Utilitities module provides commonly used functionality to the rest of
 * the application. For example, showing a messagebox with a nice format or
 * revealing files in the operating system's preffered file manager.
 */
class Utilities : public QObject
{
  Q_OBJECT

public:
  static Utilities &instance();
  static void rebootApplication();
  static QPixmap getHiDpiPixmap(const QString &path);

  Q_INVOKABLE static bool askAutomaticUpdates();
  Q_INVOKABLE static QString hdpiImagePath(const QString &path);

  // clang-format off
  static int showMessageBox(const QString& text,
                            const QString& informativeText = "",
                            QMessageBox::Icon icon = QMessageBox::Information,
                            const QString& windowTitle = "",
                            QMessageBox::StandardButtons bt = QMessageBox::Ok,
                            QMessageBox::StandardButton defaultButton = QMessageBox::NoButton,
                            const ButtonTextMap& buttonTexts = ButtonTextMap());
  // clang-format on

public slots:
  static void aboutQt();
  static void revealFile(const QString &pathToReveal);
};
} // namespace Misc
