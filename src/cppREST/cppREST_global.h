#ifndef CPPREST_GLOBAL_H
#define CPPREST_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CPPREST_LIBRARY)
#  define CPPRESTSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CPPRESTSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CPPREST_GLOBAL_H
