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
	void openPort(int port);

private slots:
	void delayedInitialization();
	void showSettings();
	void openDocumentation();
	void showAboutDialog();
	void addNRegions();

	//executes an API command
	void executeApiCommand(ApiCommand cmd);

	void hideSecondarySplit();
	void showSecondarySplit();
	void toggleSecondarySplit();

private:
	Ui::MainWindow ui_;
	DelayedInitializationTimer init_timer_;

	//API listener and methods to handle API commands
	CommandServer api_;
	void handleGoto(QStringList args);
	void handleLoad(QStringList args);
	void handleGenome(QStringList args);
	void handleViewAsPairs();
	void handleColorBy(QStringList args);
	void handleScreenshot(QStringList args);
	void handleGroupBy(QStringList args);
};

#endif // MAINWINDOW_H
