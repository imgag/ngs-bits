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
	//Returns if the analysis is to be performed with high priority
	bool highPriority() const;

private slots:
	void on_add_samples_clicked(bool);
	void updateStartButton();
	void annotate_only_state_changed();

private:
	Ui::TrioDialog ui_;
	NGSD db_;
	QList<SampleDetails> samples_;
	QList<AnalysisStep> steps_;
	QList<AnalysisStep> steps_lr_;

	//Adds a sample and returns the processed sample ID (throws an empty exception if canceled)
	QString addSample(QString status, QString sample="", bool force_showing_dialog=false);
	void updateSampleTable();
	void checkLongread();
};

#endif // TRIODIALOG_H
