

#pragma once

#include <QFile>
#include <QObject>
#include <QVector>
#include <QKeyEvent>

namespace CSV
{
/**
 * @brief The Player class
 *
 * The CSV player class allows users to select a CSV file and "re-play" it
 * with Primary Frequency Regulation Monitor.
 */
class Player : public QObject
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(bool isOpen
             READ isOpen
             NOTIFY openChanged)
  Q_PROPERTY(double progress
             READ progress
             NOTIFY timestampChanged)
  Q_PROPERTY(double frameCount
             READ frameCount
             NOTIFY playerStateChanged)
  Q_PROPERTY(double framePosition
             READ framePosition
             NOTIFY timestampChanged)
  Q_PROPERTY(bool isPlaying
             READ isPlaying
             NOTIFY playerStateChanged)
  Q_PROPERTY(const QString& timestamp
             READ timestamp
             NOTIFY timestampChanged)
  // clang-format on

signals:
  void openChanged();
  void timestampChanged();
  void playerStateChanged();

private:
  explicit Player();
  Player(Player &&) = delete;
  Player(const Player &) = delete;
  Player &operator=(Player &&) = delete;
  Player &operator=(const Player &) = delete;

public:
  static Player &instance();

  [[nodiscard]] bool isOpen() const;
  [[nodiscard]] double progress() const;
  [[nodiscard]] bool isPlaying() const;
  [[nodiscard]] int frameCount() const;
  [[nodiscard]] int framePosition() const;

  [[nodiscard]] QString filename() const;
  [[nodiscard]] const QString &timestamp() const;

public slots:
  void play();
  void pause();
  void toggle();
  void openFile();
  void closeFile();
  void nextFrame();
  void previousFrame();
  void openFile(const QString &filePath);
  void setProgress(const double progress);

private slots:
  void updateData();

private:
  bool promptUserForDateTimeOrInterval();
  void generateDateTimeForRows(int interval);
  void convertColumnToDateTime(int columnIndex);

  QDateTime getDateTime(int row);
  QDateTime getDateTime(const QString &cell);

  QByteArray getFrame(const int row);

  const QString getCellValue(const int row, const int column, bool &error);

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;
  bool handleKeyPress(QKeyEvent *keyEvent);

private:
  int m_framePos;
  bool m_playing;
  QFile m_csvFile;
  QString m_timestamp;
  QList<QStringList> m_csvData;
};
} // namespace CSV
