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
	MainWindow(QWidget* parent = 0);
	void resizeEvent(QResizeEvent* event);

public slots:
	void on_actionClose_triggered();
	void on_actionAbout_triggered();

private:
	//GUI
	Ui::MainWindow ui;
};

#endif // MAINWINDOW_H
