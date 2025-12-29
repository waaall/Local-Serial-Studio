

#pragma once

#include "Bar.h"

namespace Widgets
{
class Gauge : public Bar
{
  Q_OBJECT

public:
  explicit Gauge(const int index = -1, QQuickItem *parent = nullptr);

private slots:
  void updateData() override;
};
} // namespace Widgets
