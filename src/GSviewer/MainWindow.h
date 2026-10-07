#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CommandServer.h"
#include "ui_MainWindow.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);

private slots:
    ///Enables/Disables the remote application control API
    void on_actionEnable_Remote_Application_Control_changed();

    //APICommands
    void executeApiCommand(GSVCommand cmd);

private:
	Ui::MainWindow ui_;
    CommandServer api;

    void handleGoto(QString args);
};

#endif // MAINWINDOW_H
