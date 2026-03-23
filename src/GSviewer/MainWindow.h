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

signals:
	void loadFile(QString);

private:
	Ui::MainWindow ui_;
	QMenu* file_menu_;

	void createMenus();
	void loadFiles();
};

#endif // MAINWINDOW_H
