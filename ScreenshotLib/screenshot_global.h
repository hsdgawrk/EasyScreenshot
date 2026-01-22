#pragma once
#include <QtCore/qglobal.h>

#if defined(SCREENSHOT_LIBRARY)
#  define SCREENSHOT_EXPORT Q_DECL_EXPORT
#else
#  define SCREENSHOT_EXPORT Q_DECL_IMPORT
#endif