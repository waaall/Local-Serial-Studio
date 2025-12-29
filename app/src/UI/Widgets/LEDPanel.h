

#pragma once

#include <QVector>
#include <QQuickItem>

namespace Widgets
{
/**
 * @brief A widget that displays a panel of LEDs.
 */
class LEDPanel : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(int count READ count CONSTANT)
  Q_PROPERTY(QStringList titles READ titles CONSTANT)
  Q_PROPERTY(QVector<bool> states READ states NOTIFY updated)
  Q_PROPERTY(QStringList colors READ colors NOTIFY themeChanged)

signals:
  void updated();
  void themeChanged();

public:
  explicit LEDPanel(const int index = -1, QQuickItem *parent = nullptr);

  [[nodiscard]] int count() const;
  [[nodiscard]] const QList<bool> &states() const;
  [[nodiscard]] const QStringList &colors() const;
  [[nodiscard]] const QStringList &titles() const;

private slots:
  void updateData();
  void onThemeChanged();

private:
  int m_index;
  QVector<bool> m_states;
  QStringList m_titles;
  QStringList m_colors;
};

} // namespace Widgets
