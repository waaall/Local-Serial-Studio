

#pragma once

#include <QObject>
#include <QSettings>

namespace Misc
{
/**
 * @brief Manages the application's workspace directory.
 *
 * Handles persistent workspace path configuration, including default path
 * resolution, user path selection, and automatic subdirectory creation.
 */
class WorkspaceManager : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString path READ path NOTIFY pathChanged)
  Q_PROPERTY(QString shortPath READ shortPath NOTIFY pathChanged)

signals:
  void pathChanged();

private:
  explicit WorkspaceManager();
  WorkspaceManager(WorkspaceManager &&) = delete;
  WorkspaceManager(const WorkspaceManager &) = delete;
  WorkspaceManager &operator=(WorkspaceManager &&) = delete;
  WorkspaceManager &operator=(const WorkspaceManager &) = delete;

public:
  static WorkspaceManager &instance();

  [[nodiscard]] QString path() const;
  [[nodiscard]] QString shortPath() const;

  [[nodiscard]] QString path(const QString &subdirectory) const;

public slots:
  void reset();
  void selectPath();

private:
  QString m_path;
  QSettings m_settings;
};
} // namespace Misc
