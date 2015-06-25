#ifndef EXTERNALTOOLDIALOG_H
#define EXTERNALTOOLDIALOG_H

#include <QDialog>
#include <QProcess>
#include <QTime>
#include "ui_ExternalToolDialog.h"

class ExternalToolDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	ExternalToolDialog(QString tool_name, QString args, QWidget* parent = 0);
	QString getFileName(QString title, QString filters);
	void startTool(QString arguments);

private slots:
	void browse();
	void stateChanged(QProcess::ProcessState state);

private:
	Ui::ExternalToolDialog ui_;
	QString args_;
	QProcess* process_;
	QTime timer_;
};

#endif // EXTERNALTOOLDIALOG_H
