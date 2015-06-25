#ifndef APPLICATION_H
#define APPLICATION_H

#include "cppGUI_global.h"
#include <QApplication>

///Replacement for QApplication that handles Exceptions
class CPPGUISHARED_EXPORT Application
  : public QApplication
{
  Q_OBJECT

  public:
    Application(int& argc, char** argv);

  private:
    bool notify(QObject* rec, QEvent* ev);
};

#endif
