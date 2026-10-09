#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CommandServer.h"
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

    ///Enables/Disables the remote application control API
    void on_actionEnable_Remote_Application_Control_changed();

    //APICommands
    void executeApiCommand(GSVCommand cmd);

private:
	Ui::MainWindow ui_;
    DelayedInitializationTimer init_timer_;

    CommandServer api;

    void handleGoto(QString args);
    void handleLoad(QString args);
    void handleGenome(QString args);
    void err(QString header, QString text);
};

#endif // MAINWINDOW_H
