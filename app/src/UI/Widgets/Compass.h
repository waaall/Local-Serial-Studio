

#pragma once

#include <QQuickItem>

namespace Widgets
{
/**
 * @brief A widget that displays a compass.
 */
class Compass : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(double value READ value NOTIFY updated)
  Q_PROPERTY(QString text READ text NOTIFY updated)

signals:
  void updated();

public:
  explicit Compass(const int index = -1, QQuickItem *parent = nullptr);

  [[nodiscard]] double value() const;
  [[nodiscard]] QString text() const;

private slots:
  void updateData();

private:
  int m_index;
  double m_value;
  QString m_text;
};
} // namespace Widgets
