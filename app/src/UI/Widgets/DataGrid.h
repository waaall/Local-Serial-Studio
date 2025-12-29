

#pragma once

#include "JSON/Frame.h"
#include "UI/DeclarativeWidgets/StaticTable.h"

namespace Widgets
{
class DataGrid : public StaticTable
{
  Q_OBJECT
  Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)

signals:
  void pausedChanged();

public:
  explicit DataGrid(const int index = -1, QQuickItem *parent = nullptr);

  [[nodiscard]] bool paused() const;

public slots:
  void setPaused(const bool paused);

private slots:
  void updateData();

private:
  QStringList getRow(const JSON::Dataset &dataset);

private:
  int m_index;
  bool m_paused;
};
} // namespace Widgets
