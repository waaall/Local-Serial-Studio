

#pragma once

#include <QObject>
#include <QSettings>
#include <QQmlApplicationEngine>

#include "Platform/NativeWindow.h"

namespace Misc
{
/**
 * @brief The ModuleManager class
 *
 * The @c ModuleManager class is in charge of initializing all the C++ modules
 * that are part of Primary Frequency Regulation Monitor in the correct order.
 */
class ModuleManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool autoUpdaterEnabled READ autoUpdaterEnabled CONSTANT)
  Q_PROPERTY(bool softwareRendering READ softwareRendering WRITE
                 setSoftwareRendering NOTIFY softwareRenderingChanged)

signals:
  void softwareRenderingChanged();

public:
  ModuleManager();

  [[nodiscard]] bool softwareRendering() const;
  [[nodiscard]] bool autoUpdaterEnabled() const;
  [[nodiscard]] const QQmlApplicationEngine &engine() const;

public slots:
  void onQuit();
  void configureUpdater();
  void registerQmlTypes();
  void initializeQmlInterface();
  void setSoftwareRendering(const bool enabled);

private:
  QSettings m_settings;
  bool m_softwareRendering;
  NativeWindow m_nativeWindow;
  QQmlApplicationEngine m_engine;
};
} // namespace Misc
