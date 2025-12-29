

#pragma once

#include <QQuickItem>

namespace Widgets
{
/**
 * @brief A widget that displays the accelerometer data.
 */
class Accelerometer : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(double theta READ theta NOTIFY updated)
  Q_PROPERTY(double magnitude READ magnitude NOTIFY updated)

signals:
  void updated();

public:
  explicit Accelerometer(const int index = -1, QQuickItem *parent = nullptr);

  [[nodiscard]] double theta() const;
  [[nodiscard]] double magnitude() const;

private slots:
  void updateData();

private:
  int m_index;
  double m_theta;
  double m_magnitude;
};

} // namespace Widgets
