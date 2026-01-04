

#include "ConsoleExport.h"

#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QApplication>
#include <QDesktopServices>

#include "Misc/Utilities.h"

#ifdef BUILD_COMMERCIAL
#  include "IO/Console.h"
#  include "IO/Manager.h"
#  include "Misc/TimerEvents.h"
#  include "Misc/WorkspaceManager.h"
#  include "Licensing/LemonSqueezy.h"
#endif

/**
 * Constructor function, configures the path in which Primary Frequency Regulation Monitor shall
 * automatically write generated console log files.
 */
IO::ConsoleExport::ConsoleExport()
  : m_exportEnabled(false)
{
#ifdef BUILD_COMMERCIAL
  connect(&Licensing::LemonSqueezy::instance(),
          &Licensing::LemonSqueezy::activatedChanged, this, [=, this] {
            if (exportEnabled() && !SerialStudio::activated())
              setExportEnabled(false);
          });
#endif

  setExportEnabled(m_settings.value("ConsoleExport", false).toBool());
}

/**
 * Close file & finnish write-operations before destroying the class.
 */
IO::ConsoleExport::~ConsoleExport()
{
  closeFile();
}

/**
 * Returns a pointer to the only instance of this class.
 */
IO::ConsoleExport &IO::ConsoleExport::instance()
{
  static ConsoleExport instance;
  return instance;
}

/**
 * Returns @c true if the console output file is open.
 */
bool IO::ConsoleExport::isOpen() const
{
#ifdef BUILD_COMMERCIAL
  return m_file.isOpen();
#else
  return false;
#endif
}

/**
 * Returns @c true if console log export is enabled.
 */
bool IO::ConsoleExport::exportEnabled() const
{
#ifdef BUILD_COMMERCIAL
  return m_exportEnabled;
#else
  return false;
#endif
}

/**
 * Write all remaining console data & close the output file.
 */
void IO::ConsoleExport::closeFile()
{
#ifdef BUILD_COMMERCIAL
  if (isOpen())
  {
    if (m_buffer.size() > 0)
      writeData();

    m_file.close();
    m_textStream.setDevice(nullptr);

    Q_EMIT openChanged();
  }
#endif
}

/**
 * Configures the signal/slot connections with the modules of the application
 * that this module depends upon.
 */
void IO::ConsoleExport::setupExternalConnections()
{
#ifdef BUILD_COMMERCIAL
  connect(&IO::Console::instance(), &IO::Console::displayString, this,
          &IO::ConsoleExport::registerData);
  connect(&IO::Manager::instance(), &IO::Manager::connectedChanged, this,
          &IO::ConsoleExport::closeFile);
  connect(&Misc::TimerEvents::instance(), &Misc::TimerEvents::timeout1Hz, this,
          &IO::ConsoleExport::writeData);
#endif
}

/**
 * Enables or disables data export.
 */
void IO::ConsoleExport::setExportEnabled(const bool enabled)
{
  m_exportEnabled = enabled;
  Q_EMIT enabledChanged();

  if (!exportEnabled() && isOpen())
  {
    m_buffer.clear();
    closeFile();
  }

  m_settings.setValue("ConsoleExport", m_exportEnabled);
}

/**
 * Writes current buffer data to the output file, and creates a new file
 * if needed.
 */
void IO::ConsoleExport::writeData()
{
#ifdef BUILD_COMMERCIAL
  // Device not connected, abort
  if (!IO::Manager::instance().isConnected())
    return;

  // Export is disabled, abort
  if (!exportEnabled())
    return;

  // Write data to the file
  if (m_buffer.size() > 0)
  {
    // Create a new file if required
    if (!isOpen())
      createFile();

    // Write data to hard disk
    if (m_textStream.device())
    {
      m_textStream << m_buffer;
      m_textStream.flush();
      m_buffer.clear();
    }
  }
#endif
}

/**
 * Creates a new console log output file based on the current date/time.
 */
void IO::ConsoleExport::createFile()
{
#ifdef BUILD_COMMERCIAL
  // Only enable this feature is program is activated
  if (SerialStudio::activated())
  {
    // Close current file (if any)
    if (isOpen())
      closeFile();

    // Get filename
    const auto dateTime = QDateTime::currentDateTime();
    const auto fileName
        = dateTime.toString(QStringLiteral("yyyy_MMM_dd HH_mm_ss"))
          + QStringLiteral(".txt");

    // Get console export path
    QDir dir(Misc::WorkspaceManager::instance().path("Console"));

    // Open file
    m_file.setFileName(dir.filePath(fileName));
    if (!m_file.open(QIODeviceBase::WriteOnly | QIODevice::Text))
    {
      Misc::Utilities::showMessageBox(tr("Console Output File Error"),
                                      tr("Cannot open file for writing!"),
                                      QMessageBox::Critical);
      closeFile();
      return;
    }

    // Configure text stream
    m_textStream.setDevice(&m_file);
    m_textStream.setGenerateByteOrderMark(true);
#  if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_textStream.setCodec("UTF-8");
#  else
    m_textStream.setEncoding(QStringConverter::Utf8);
#  endif

    // Emit signals
    Q_EMIT openChanged();
  }
#endif
}

/**
 * Appends the given console data to the output buffer.
 */
void IO::ConsoleExport::registerData(QStringView data)
{
#ifdef BUILD_COMMERCIAL
  if (!data.isEmpty() && exportEnabled())
    m_buffer.append(data);
#else
  (void)data;
#endif
}
