#ifndef CPPCORE_GLOBAL_H
#define CPPCORE_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CPPCORE_LIBRARY)
#  define CPPCORESHARED_EXPORT Q_DECL_EXPORT
#else
#  define CPPCORESHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CPPCORE_GLOBAL_H
