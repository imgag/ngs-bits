#ifndef CPPGUI_GLOBAL_H
#define CPPGUI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CPPGUI_LIBRARY)
#  define CPPGUISHARED_EXPORT Q_DECL_EXPORT
#else
#  define CPPGUISHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CPPGUI_GLOBAL_H
