#ifndef SINGLESAMPLEANALYSISDIALOG_H
#define SINGLESAMPLEANALYSISDIALOG_H

#include <QDialog>
#include "NGSD.h"
#include "ui_SingleSampleAnalysisDialog.h"

struct SampleDetails
{
	QString name;
	QString system;
	QString status;
	QString quality;
	QString gender;
};

struct AnalysisStep
{
	QString name;
	QString description;
};

//Dialog for starting a single sample analysis.
class SingleSampleAnalysisDialog
	: public QDialog
{
	Q_OBJECT

public:
	SingleSampleAnalysisDialog(QWidget *parent = 0);
	//Fills table with given processed samples
	void setSamples(QList<AnalysisJobSample> samples);

	//Returns the processed sample list.
	QList<AnalysisJobSample> samples() const;
	//Returns the command line arguments.
	QStringList arguments() const;
	//Returns if the analysis is to be performed with high priority
	bool highPriority() const;

	static void addSample(NGSD& db, QString status, QList<SampleDetails>& samples, QString sample="", bool throw_if_bam_missing=true);
	static void updateSampleTable(const QList<SampleDetails>& samples, QTableWidget* samples_table);
	static QList<AnalysisJobSample> samples(const QList<SampleDetails>& samples);
	static QList<AnalysisStep> loadSteps(QString ini_name);
	static void addStepsToParameters(QList<AnalysisStep> steps, QFormLayout* layout);
	static QStringList arguments(const QWidget* widget);

private slots:
	void on_add_sample_clicked(bool);
	void on_add_batch_clicked(bool);
	void on_clear_clicked(bool);
	void updateStartButton();

private:
	Ui::SingleSampleAnalysisDialog ui_;
	NGSD db_;
	QList<SampleDetails> samples_;
	QList<AnalysisStep> steps_;

	void addSample(QString status, QString sample="");
	void updateSampleTable();
};

#endif // SINGLESAMPLEANALYSISDIALOG_H
