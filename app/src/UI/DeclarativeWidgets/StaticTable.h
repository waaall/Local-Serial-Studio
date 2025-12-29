

#pragma once

#include <QTableWidget>
#include <QStyledItemDelegate>

#include "DeclarativeWidget.h"

class StaticTable : public DeclarativeWidget
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(const QFont& font
             READ font
             WRITE setFont
             NOTIFY fontChanged)
  Q_PROPERTY(const QFont& headerFont
             READ headerFont
             WRITE setHeaderFont
             NOTIFY headerFontChanged)
  // clang-format on

Q_SIGNALS:
  void fontChanged();
  void headerFontChanged();

public:
  explicit StaticTable(QQuickItem *parent = nullptr);

  [[nodiscard]] const QFont &font() const;
  [[nodiscard]] const QFont &headerFont() const;
  [[nodiscard]] const QList<QStringList> &data() const;

public Q_SLOTS:
  void setFont(const QFont &font);
  void setHeaderFont(const QFont &font);
  void setData(const QList<QStringList> &data);

private Q_SLOTS:
  void loadTheme();

private:
  QFont m_font;
  QFont m_headerFont;
  QTableWidget m_widget;
  QList<QStringList> m_data;
};
