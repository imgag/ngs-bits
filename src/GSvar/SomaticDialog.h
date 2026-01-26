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

	///sets custom arguments to given QByteArray
	void setCustomArguments(const QString& args);

	//Returns the processed sample list.
	QList<AnalysisJobSample> samples() const;
	//Returns the command line arguments.
	QStringList arguments() const;
	//Returns if the analysis is to be performed with high priority
	bool highPriority() const;
	//Returns if the analysis is to be performed with illumina DRAGEN pipeline
	bool useDragen() const;

private slots:
	void on_add_samples_clicked(bool);
	void updateStartButton();

private:
	Ui::SomaticDialog ui_;
	NGSD db_;
	QList<SampleDetails> samples_;
	QList<AnalysisStep> steps_;

	QString addSample(QString status, QString sample="", bool force_showing_dialog=false);
	void updateSampleTable();
};

#endif // SOMATICDIALOG_H
