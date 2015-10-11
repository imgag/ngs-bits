#include "MainWindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui()
{
	ui.setupUi(this);
	setWindowTitle(QApplication::applicationName());
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
	QMainWindow::resizeEvent(event);
	ui.splitter->setSizes(QList<int>() << 250 << width()-250);
}

void MainWindow::on_actionOpen_triggered()
{
	//TODO
}

void MainWindow::on_actionClose_triggered()
{
	close();
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About " + QCoreApplication::applicationName(), QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion()+ "\n\nAn MID mixing tool for NGS sequencing");
}
