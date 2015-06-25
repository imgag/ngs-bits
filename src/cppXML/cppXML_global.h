#ifndef CPPXML_GLOBAL_H
#define CPPXML_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CPPXML_LIBRARY)
#  define CPPXMLSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CPPXMLSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CPPXML_GLOBAL_H
