

#pragma once

#include <QQuickItem>

#include "SerialStudio.h"

namespace UI
{
/**
 * @brief The DashboardWidget class
 *
 * The DashboardWidget class serves as a container and manager for various
 * dashboard widgets in a QML interface. It dynamically creates and manages
 * QQuickItem-based widgets (including QQuickPaintedItems) based on the widget
 * type specified by the UI::Dashboard class.
 *
 * This class handles the creation, sizing, and visibility of widgets, as well
 * as providing a consistent interface for different widget types to the QML
 * layer. It supports both QQuickItem and QQuickPaintedItem derived widgets,
 * allowing for flexible and efficient rendering strategies.
 *
 * The class also manages special cases like GPS widgets and external windows,
 * providing properties and methods to handle these scenarios.
 */
class DashboardWidget : public QQuickItem
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(int widgetIndex
             READ widgetIndex
             WRITE setWidgetIndex
             NOTIFY widgetIndexChanged)
  Q_PROPERTY(int relativeIndex
             READ relativeIndex
             NOTIFY widgetIndexChanged)
  Q_PROPERTY(QColor widgetColor
             READ widgetColor
             NOTIFY widgetColorChanged)
  Q_PROPERTY(QString widgetTitle
             READ widgetTitle
             NOTIFY widgetIndexChanged)
  Q_PROPERTY(QString widgetQmlPath
             READ widgetQmlPath
             NOTIFY widgetIndexChanged)
  Q_PROPERTY(QQuickItem* widgetModel
             READ widgetModel
             NOTIFY widgetIndexChanged)
  Q_PROPERTY(SerialStudio::DashboardWidget widgetType
             READ widgetType
             NOTIFY widgetIndexChanged)
  // clang-format on

signals:
  void widgetIndexChanged();
  void widgetColorChanged();

public:
  DashboardWidget(QQuickItem *parent = 0);
  ~DashboardWidget();

  [[nodiscard]] int widgetIndex() const;
  [[nodiscard]] int relativeIndex() const;
  [[nodiscard]] QColor widgetColor() const;
  [[nodiscard]] QString widgetTitle() const;
  [[nodiscard]] SerialStudio::DashboardWidget widgetType() const;

  [[nodiscard]] QString widgetQmlPath() const;
  [[nodiscard]] QQuickItem *widgetModel() const;

public slots:
  void setWidgetIndex(const int index);

private:
  int m_index;
  int m_relativeIndex;
  SerialStudio::DashboardWidget m_widgetType;

  QString m_qmlPath;
  QQuickItem *m_dbWidget;
};
} // namespace UI
