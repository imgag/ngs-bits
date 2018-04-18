#ifndef SOMATICDIALOG_H
#define SOMATICDIALOG_H

#include <QDialog>
#include "ui_SomaticDialog.h"
#include "SingleSampleAnalysisDialog.h"

//Dialog for starting a somatic tumor-normal analysis.
class SomaticDialog
		: public QDialog
{
	Q_OBJECT

public:
	explicit SomaticDialog(QWidget* parent = 0);
	//Fills table with given processed samples
	void setSamples(QList<AnalysisJobSample> samples);

	//Returns the processed sample list.
	QList<AnalysisJobSample> samples() const;
	//Returns the command line arguments.
	QStringList arguments() const;
	//Returns if the analysis is to be performed with high priority
	bool highPriority() const;

private slots:
	void on_add_samples_clicked(bool);
	void updateStartButton();

private:
	Ui::SomaticDialog ui_;
	NGSD db_;
	QList<SampleDetails> samples_;
	QList<AnalysisStep> steps_;

	void addSample(QString status, QString sample="");
	void updateSampleTable();
};

#endif // SOMATICDIALOG_H
