

#pragma once

#include <QFile>
#include <QObject>
#include <QSettings>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>

#include "SerialStudio.h"

#include "JSON/Frame.h"
#include "JSON/FrameParser.h"

namespace JSON
{
/**
 * @brief The FrameBuilder class
 *
 * The JSON frame builder class receives raw frame data from the I/O manager
 * class and generates a frame object that represents the project title,
 * the groups that compose the project and the datasets that compose each
 * group.
 *
 * This frame is later shared with the rest of the modules, and is updated
 * automatically with new incoming raw data.
 */
class FrameBuilder : public QObject
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(QString jsonMapFilename
             READ jsonMapFilename
             NOTIFY jsonFileMapChanged)
  Q_PROPERTY(QString jsonMapFilepath
             READ jsonMapFilepath
             NOTIFY jsonFileMapChanged)
  Q_PROPERTY(SerialStudio::OperationMode operationMode
             READ operationMode
             WRITE setOperationMode
             NOTIFY operationModeChanged)
  // clang-format on

signals:
  void jsonFileMapChanged();
  void operationModeChanged();
  void frameChanged(const JSON::Frame &frame);

private:
  explicit FrameBuilder();
  FrameBuilder(FrameBuilder &&) = delete;
  FrameBuilder(const FrameBuilder &) = delete;
  FrameBuilder &operator=(FrameBuilder &&) = delete;
  FrameBuilder &operator=(const FrameBuilder &) = delete;

public:
  static FrameBuilder &instance();

  [[nodiscard]] QString jsonMapFilepath() const;
  [[nodiscard]] QString jsonMapFilename() const;
  [[nodiscard]] const JSON::Frame &frame() const;
  [[nodiscard]] JSON::FrameParser *frameParser() const;
  [[nodiscard]] SerialStudio::OperationMode operationMode() const;

public slots:
  void setupExternalConnections();
  void loadJsonMap(const QString &path);
  void setFrameParser(JSON::FrameParser *editor);
  void setOperationMode(const SerialStudio::OperationMode mode);

  void hotpathRxFrame(const QByteArray &data);

private slots:
  void onConnectedChanged();

private:
  void setJsonPathSetting(const QString &path);

  void parseProjectFrame(const QByteArray &data);
  void parseQuickPlotFrame(const QByteArray &data);
  void buildQuickPlotFrame(const QStringList &channels);

  void hotpathTxFrame(const JSON::Frame &frame);

private:
  QFile m_jsonMap;

  JSON::Frame m_frame;
  JSON::Frame m_rawFrame;
  JSON::Frame m_quickPlotFrame;

  QString m_checksum;
  QByteArray m_frameStart;
  QByteArray m_frameFinish;

  QSettings m_settings;
  int m_quickPlotChannels;
  JSON::FrameParser *m_frameParser;
  SerialStudio::OperationMode m_opMode;
};
} // namespace JSON
