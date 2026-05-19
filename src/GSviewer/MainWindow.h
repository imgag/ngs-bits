#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_MainWindow.h"
#include "GenomeData.h"

///Main window class
class MainWindow
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MainWindow(QWidget* parent = 0);


private:
    Ui::MainWindow ui_;
    QSharedPointer<GenomeData> genome_data_;
};

#endif // MAINWINDOW_H
