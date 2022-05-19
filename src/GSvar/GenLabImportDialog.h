#ifndef GENLABIMPORTDIALOG_H
#define GENLABIMPORTDIALOG_H

#include <QDialog>
#include "ui_GenLabImportDialog.h"
#include "NGSD.h"

//Dialog for import of data from GenLab
class GenLabImportDialog
	: public QDialog
{
	Q_OBJECT

public:
	//Constructor
	GenLabImportDialog(QString ps_id, QWidget* parent);

	//imports the selected data
	void importSelected();

protected slots:
	void initTable();

protected:
	void addItem(QString label, QString v_ngsd, QString v_genlab);
	bool diseaseDataContains(QString sample_id, QString type, QString value);
	bool relationDataContains(QString sample_id, SampleRelation relation);

private:
	Ui::GenLabImportDialog ui_;
	NGSD db_;
	QString ps_id_;
};

#endif // GENLABIMPORTDIALOG_H
