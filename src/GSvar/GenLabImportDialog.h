#ifndef GENLABIMPORTDIALOG_H
#define GENLABIMPORTDIALOG_H

#include <QDialog>
#include "ui_GenLabImportDialog.h"
#include "NGSD.h"
#include "DelayedInitializationTimer.h"

//Dialog for import of data from GenLab
class GenLabImportDialog
	: public QDialog
{
	Q_OBJECT

public:
	//Constructor
	GenLabImportDialog(QString ps_id, QWidget* parent);

	//imports the selected data
	void importSelectedData();

protected slots:
	void delayedInitialization();
	void initTable();

protected:
	void addItem(QString label, QString v_ngsd, QString v_genlab);
	bool diseaseDataContains(QString sample_id, QString type, QString value);
	bool relationDataContains(QString sample_id, SampleRelation relation);
	bool studyDataContains(QString ps_id, QString study);
	static void addDiseaseDetail(QList<SampleDiseaseInfo>& disease_details, QString type, QString data);

private:
	Ui::GenLabImportDialog ui_;
	DelayedInitializationTimer init_timer_;
	NGSD db_;
	QString ps_id_;
};

#endif // GENLABIMPORTDIALOG_H
