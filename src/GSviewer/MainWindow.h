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
    MainWindow(QString bind, QWidget* parent = 0);

private slots:
	void delayedInitialization();
	void showSettings();
	void openDocumentation();
	void showAboutDialog();
	void addNRegions();
	void showNamedColors();

    ///Enables/Disables the remote application control API
    void on_actionEnable_Remote_Application_Control_changed();

    //APICommands
    void executeApiCommand(ApiCommand cmd);

private:
	Ui::MainWindow ui_;
    DelayedInitializationTimer init_timer_;

    CommandServer api;

    void err(QString header, QString text);
    void handleGoto(QStringList args);
    void handleLoad(QStringList args);
    void handleGenome(QStringList args);
    void handleSleep(QStringList args);
    void handleViewAsPairs(QStringList args);
    void handleColorBy(QStringList args);
    void handleScreenshot(QStringList args);
    void handleGroupBy(QStringList args);
};

#endif // MAINWINDOW_H
