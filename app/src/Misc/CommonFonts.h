

#pragma once

#include <QFont>
#include <QObject>

namespace Misc
{
/**
 * @class Misc::CommonFonts
 * @brief A class providing common fonts for the user interface.
 */
class CommonFonts : public QObject
{
  // clang-format off
  Q_OBJECT
  Q_PROPERTY(const QFont& uiFont
             READ uiFont
             NOTIFY fontsChanged)
  Q_PROPERTY(const QFont& monoFont
             READ monoFont
             NOTIFY fontsChanged)
  Q_PROPERTY(const QFont& boldUiFont
             READ boldUiFont
             NOTIFY fontsChanged)
  // clang-format on

signals:
  void fontsChanged();

private:
  explicit CommonFonts();
  CommonFonts(CommonFonts &&) = delete;
  CommonFonts(const CommonFonts &) = delete;
  CommonFonts &operator=(CommonFonts &&) = delete;
  CommonFonts &operator=(const CommonFonts &) = delete;

public:
  static CommonFonts &instance();

  [[nodiscard]] const QFont &uiFont() const;
  [[nodiscard]] const QFont &monoFont() const;
  [[nodiscard]] const QFont &boldUiFont() const;

  Q_INVOKABLE QFont customUiFont(double fraction = 1, bool bold = false);
  Q_INVOKABLE QFont customMonoFont(double fraction = 1, bool bold = false);

private:
  QFont m_uiFont;
  QFont m_monoFont;
  QFont m_boldUiFont;
};
} // namespace Misc
