#ifndef CPPNGSD_GLOBAL_H
#define CPPNGSD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CPPNGSD_LIBRARY)
#  define CPPNGSDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CPPNGSDSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CPPNGSD_GLOBAL_H
