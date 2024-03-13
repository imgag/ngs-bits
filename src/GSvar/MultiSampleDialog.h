#ifndef MULTISAMPLEDIALOG_H
#define MULTISAMPLEDIALOG_H

#include <QDialog>
#include "ui_MultiSampleDialog.h"
#include "SingleSampleAnalysisDialog.h"

//Dialog for starting a multi-sample analysis.
class MultiSampleDialog
	: public QDialog
{
	Q_OBJECT

public:
	MultiSampleDialog(QWidget* parent = 0);
	//Fills table with given processed samples
	void setSamples(QList<AnalysisJobSample> samples);

	//Returns the processed sample list.
	QList<AnalysisJobSample> samples() const;
	//Returns the command line arguments.
	QStringList arguments() const;
	//Returns if the analysis is to be performed with high priority
	bool highPriority() const;

private slots:
	void on_add_control_clicked(bool);
	void on_add_affected_clicked(bool);
	void on_clear_clicked(bool);
	void updateStartButton();
	void annotate_only_state_changed();

private:
	Ui::MultiSampleDialog ui_;
	NGSD db_;
	QList<SampleDetails> samples_;
	QList<AnalysisStep> steps_;
	QList<AnalysisStep> steps_lr_;

	void addSample(QString status, QString sample="");
	void updateSampleTable();
	void checkLongread();
};

#endif // MULTISAMPLEDIALOG_H
