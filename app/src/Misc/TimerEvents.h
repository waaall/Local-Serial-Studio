

#pragma once

#include <QObject>
#include <QSettings>
#include <QBasicTimer>

namespace Misc
{
/**
 * @class TimerEvents
 * @brief Singleton class providing multiple periodic timer signals.
 *
 * TimerEvents manages several QBasicTimer instances that emit signals
 * at fixed or configurable rates. It is designed for use across the
 * application to provide consistent periodic events without duplicating
 * timer logic in multiple components.
 *
 * Timers provided:
 * - 1 Hz timer (timeout1Hz())
 * - 10 Hz timer (timeout10Hz())
 * - 20 Hz timer (timeout20Hz())
 * - UI timer (uiTimeout()) — frequency configurable between 1 and 240 Hz
 *
 * The UI timer frequency is stored in persistent settings under the key
 * "uiTimerHz" and restored on construction.
 *
 * @note This class cannot be copied or moved.
 *
 * Typical usage:
 * @code
 * connect(&Misc::TimerEvents::instance(), &Misc::TimerEvents::timeout1Hz,
 *         this, &MyClass::updateOncePerSecond);
 *
 * Misc::TimerEvents::instance().setUiTimerHz(30);
 * Misc::TimerEvents::instance().startTimers();
 * @endcode
 */
class TimerEvents : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int fps READ fps WRITE setFPS NOTIFY fpsChanged)

signals:
  void uiTimeout();
  void fpsChanged();
  void timeout1Hz();
  void timeout10Hz();
  void timeout20Hz();

private:
  TimerEvents();
  TimerEvents(TimerEvents &&) = delete;
  TimerEvents(const TimerEvents &) = delete;
  TimerEvents &operator=(TimerEvents &&) = delete;
  TimerEvents &operator=(const TimerEvents &) = delete;

public:
  static TimerEvents &instance();

  [[nodiscard]] int fps() const;

protected:
  void timerEvent(QTimerEvent *event) override;

public slots:
  void stopTimers();
  void startTimers();
  void setFPS(int hz);

private:
  int m_uiTimerHz;
  QSettings m_settings;

  QBasicTimer m_uiTimer;
  QBasicTimer m_timer1Hz;
  QBasicTimer m_timer10Hz;
  QBasicTimer m_timer20Hz;
};
} // namespace Misc
