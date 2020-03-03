#ifndef PROCESSEDSAMPLEDATADELETIONDIALOG_H
#define PROCESSEDSAMPLEDATADELETIONDIALOG_H

#include <QDialog>
#include "NGSD.h"
#include "ui_ProcessedSampleDataDeletionDialog.h"


class ProcessedSampleDataDeletionDialog
	: public QDialog
{
	Q_OBJECT

public:
	ProcessedSampleDataDeletionDialog(QWidget* parent, QStringList ids);

protected slots:
	void deleteData();

signals:
	void somRepDeleted();

private:
	Ui::ProcessedSampleDataDeletionDialog ui_;
	QStringList ps_ids_;

	///returns ID of normal processed sample id
	QString matchedNormalPsID(NGSD& db, QString tumor_ps_id)
	{
		QString ps_normal_name = db.getProcessedSampleData(tumor_ps_id).normal_sample_name;
		if(ps_normal_name == "") return "";

		return db.processedSampleId(ps_normal_name);
	}
};

#endif // PROCESSEDSAMPLEDATADELETIONDIALOG_H
