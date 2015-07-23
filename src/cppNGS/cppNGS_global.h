#ifndef CPPNGS_GLOBAL_H
#define CPPNGS_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CPPNGS_LIBRARY)
#  define CPPNGSSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CPPNGSSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CPPNGS_GLOBAL_H
