#ifndef PROCESSEDSAMPLEDATADELETIONDIALOG_H
#define PROCESSEDSAMPLEDATADELETIONDIALOG_H

#include <QDialog>
#include "ui_ProcessedSampleDataDeletionDialog.h"


class ProcessedSampleDataDeletionDialog
	: public QDialog
{
	Q_OBJECT

public:
	ProcessedSampleDataDeletionDialog(QWidget* parent, QStringList ids);

protected slots:
	void deleteData();

private:
	Ui::ProcessedSampleDataDeletionDialog ui_;
	QStringList ps_ids_;
};

#endif // PROCESSEDSAMPLEDATADELETIONDIALOG_H
