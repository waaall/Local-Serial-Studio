

#pragma once

#include <qglobal.h>

// clang-format off
#define APP_NAME        PROJECT_DISPNAME
#define APP_VERSION     PROJECT_VERSION
#define APP_DEVELOPER   PROJECT_VENDOR
#define APP_SUPPORT_URL PROJECT_CONTACT
#define APP_UPDATER_URL PROJECT_APPCAST
// clang-format on

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#  define APP_EXECUTABLE QStringLiteral("Primary-Frequency-Regulation-Monitor")
#else
#  define APP_EXECUTABLE QStringLiteral("primary-frequency-regulation-monitor")
#endif
