#ifndef TRIODIALOG_H
#define TRIODIALOG_H

#include <QDialog>
#include "ui_TrioDialog.h"
#include "SingleSampleAnalysisDialog.h"

//Dialog for starting a trio analysis.
class TrioDialog
		: public QDialog
{
	Q_OBJECT

public:
	explicit TrioDialog(QWidget* parent = 0);
	//Fills table with given processed samples
	void setSamples(QList<AnalysisJobSample> samples);

	//Returns the processed sample list.
	QList<AnalysisJobSample> samples() const;
	//Returns the command line arguments.
	QStringList arguments() const;

private slots:
	void on_add_samples_clicked(bool);
	void updateStartButton();

private:
	Ui::TrioDialog ui_;
	NGSD db_;
	QList<SampleDetails> samples_;
	QList<AnalysisStep> steps_;

	void addSample(QString status, QString sample="");
	void updateSampleTable();
};

#endif // TRIODIALOG_H
