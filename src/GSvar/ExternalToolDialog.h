#ifndef EXTERNALTOOLDIALOG_H
#define EXTERNALTOOLDIALOG_H

#include <QDialog>
#include "ui_ExternalToolDialog.h"

class ExternalToolDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	ExternalToolDialog(QString tool_name, QString mode, QWidget* parent = 0);
	QString getFileName(QString title, QString filters);

private slots:
	void browse();

private:
	Ui::ExternalToolDialog ui_;
	QString tool_name_;
	QString mode_;
};

#endif // EXTERNALTOOLDIALOG_H
