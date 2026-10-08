#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "DelayedInitializationTimer.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);

private slots:
	void delayedInitialization();
	void showAboutDialog();

private:
	Ui::MainWindow ui_;
	DelayedInitializationTimer init_timer_;

};

#endif // MAINWINDOW_H
