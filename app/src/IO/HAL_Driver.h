

#pragma once

#include <QObject>
#include <QIODevice>

namespace IO
{
/**
 * @class HAL_Driver
 * @brief Abstract base class for hardware abstraction layer drivers.
 *
 * This interface defines the core API for all I/O drivers in the system.
 * It provides methods for opening, closing, and checking the state of the
 * device, as well as sending and receiving data.
 *
 * The class includes a high-efficiency buffered data processing mechanism
 * through `processData()`, which reduces signal overhead in high-frequency
 * environments by aggregating data until a configurable buffer threshold is
 * reached.
 *
 * Once the threshold is met, the data is emitted via `dataReceived()`.
 *
 * Subclasses must implement the pure virtual methods to handle specific device
 * protocols and behavior.
 *
 * Thread safety is ensured for buffer operations, making `processData()` safe
 * to call from multiple threads.
 *
 * @note Emitting signals across threads requires connected slots to handle
 *       queued events properly.
 *
 * @see setBufferSize(), processData(), flushBuffer()
 */
class HAL_Driver : public QObject
{
  Q_OBJECT

signals:
  /**
   * @brief Emitted when the driver configuration changes.
   */
  void configurationChanged();

  /**
   * @brief Emitted after data is sent.
   * @param data The raw data that was sent.
   */
  void dataSent(const QByteArray &data);

  /**
   * @brief Emitted when buffered data is ready.
   * @param data The buffered data.
   */
  void dataReceived(const QByteArray &data);

public:
  /**
   * @brief Constructor.
   * @param parent Optional QObject parent.
   */
  explicit HAL_Driver(QObject *parent = nullptr)
    : QObject(parent)
  {
  }

  /**
   * @brief Virtual destructor.
   */
  virtual ~HAL_Driver() = default;

  /**
   * @brief Close the driver.
   */
  virtual void close() = 0;

  /**
   * @brief Check if the device is open.
   * @return True if open.
   */
  [[nodiscard]] virtual bool isOpen() const = 0;

  /**
   * @brief Check if the device is readable.
   * @return True if readable.
   */
  [[nodiscard]] virtual bool isReadable() const = 0;

  /**
   * @brief Check if the device is writable.
   * @return True if writable.
   */
  [[nodiscard]] virtual bool isWritable() const = 0;

  /**
   * @brief Check if the current configuration is valid.
   * @return True if configuration is OK.
   */
  [[nodiscard]] virtual bool configurationOk() const = 0;

  /**
   * @brief Write data to the device.
   * @param data The data to write.
   * @return Number of bytes successfully written.
   */
  [[nodiscard]] virtual quint64 write(const QByteArray &data) = 0;

  /**
   * @brief Open the device with the given mode.
   * @param mode The open mode.
   * @return True if successfully opened.
   */
  [[nodiscard]] virtual bool open(const QIODevice::OpenMode mode) = 0;
};
} // namespace IO
