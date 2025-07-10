#ifndef EXPORTHISTORYDIALOG_H
#define EXPORTHISTORYDIALOG_H

#include <QDialog>
#include "ui_ExportHistoryDialog.h"

class ExportHistoryDialog
	: public QDialog
{
	Q_OBJECT

public:
	 ExportHistoryDialog(QWidget* parent, QString cm_id, QString network);

private:
	Ui::ExportHistoryDialog ui_;
	QString cm_id_;

	void updateTable();
	void addTableRow(QString data_center, QDate date, QString type, QString tan, QString status, QString submission_id, QString submission_output);
};

#endif // EXPORTHISTORYDIALOG_H
