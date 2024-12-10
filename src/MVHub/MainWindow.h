#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_MainWindow.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);

public slots:
	void test_apiConsent();
	void test_apiPseudo();
	void test_apiReCapCaseManagement();

private:
	Ui::MainWindow ui_;
	void clearOutput(QObject* sender);
};

#endif // MAINWINDOW_H
