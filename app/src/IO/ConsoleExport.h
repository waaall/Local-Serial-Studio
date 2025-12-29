

#pragma once

#include <QFile>
#include <QObject>
#include <QSettings>
#include <QTextStream>

namespace IO
{
class ConsoleExport : public QObject
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(bool isOpen
             READ isOpen
             NOTIFY openChanged)
  Q_PROPERTY(bool exportEnabled
             READ exportEnabled
             WRITE setExportEnabled
             NOTIFY enabledChanged)
  // clang-format on

signals:
  void openChanged();
  void enabledChanged();

private:
  explicit ConsoleExport();
  ConsoleExport(ConsoleExport &&) = delete;
  ConsoleExport(const ConsoleExport &) = delete;
  ConsoleExport &operator=(ConsoleExport &&) = delete;
  ConsoleExport &operator=(const ConsoleExport &) = delete;

  ~ConsoleExport();

public:
  static ConsoleExport &instance();

  [[nodiscard]] bool isOpen() const;
  [[nodiscard]] bool exportEnabled() const;

public slots:
  void closeFile();
  void setupExternalConnections();
  void setExportEnabled(const bool enabled);

private slots:
  void writeData();
  void createFile();
  void registerData(QStringView data);

private:
  QFile m_file;
  QString m_buffer;
  bool m_exportEnabled;
  QSettings m_settings;
  QTextStream m_textStream;
};
} // namespace IO
