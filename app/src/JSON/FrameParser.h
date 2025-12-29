

#pragma once

#include <QEvent>
#include <QPainter>
#include <QJSValue>
#include <QJSEngine>
#include <QCodeEditor>
#include <QSyntaxStyle>
#include <QQuickPaintedItem>

namespace JSON
{
class FrameParser : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(QString text READ text NOTIFY textChanged)
  Q_PROPERTY(bool isModified READ isModified NOTIFY modifiedChanged)
  Q_PROPERTY(bool undoAvailable READ undoAvailable NOTIFY modifiedChanged)
  Q_PROPERTY(bool redoAvailable READ redoAvailable NOTIFY modifiedChanged)

signals:
  void textChanged();
  void modifiedChanged();

public:
  FrameParser(QQuickItem *parent = 0);

  static const QString &defaultCode();

  [[nodiscard]] QString text() const;
  [[nodiscard]] bool isModified() const;

  [[nodiscard]] QStringList parse(const QString &frame);
  [[nodiscard]] QStringList parse(const QByteArray &frame);

  [[nodiscard]] bool undoAvailable() const;
  [[nodiscard]] bool redoAvailable() const;
  [[nodiscard]] bool save(const bool silent = false);
  [[nodiscard]] bool loadScript(const QString &script);

public slots:
  void cut();
  void undo();
  void redo();
  void help();
  void copy();
  void paste();
  void apply();
  void reload();
  void import();
  void readCode();
  void selectAll();

private slots:
  void onThemeChanged();

private slots:
  void renderWidget();
  void resizeWidget();

private:
  virtual void paint(QPainter *painter) override;
  virtual void keyPressEvent(QKeyEvent *event) override;
  virtual void keyReleaseEvent(QKeyEvent *event) override;
  virtual void inputMethodEvent(QInputMethodEvent *event) override;
  virtual void focusInEvent(QFocusEvent *event) override;
  virtual void focusOutEvent(QFocusEvent *event) override;
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
  virtual void wheelEvent(QWheelEvent *event) override;
  virtual void dragEnterEvent(QDragEnterEvent *event) override;
  virtual void dragMoveEvent(QDragMoveEvent *event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
  virtual void dropEvent(QDropEvent *event) override;

private:
  QPixmap m_pixmap;
  QJSEngine m_engine;
  QSyntaxStyle m_style;
  QCodeEditor m_widget;
  QJSValue m_parseFunction;
};
} // namespace JSON
