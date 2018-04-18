#ifndef ANALYSISSTATUSDIALOG_H
#define ANALYSISSTATUSDIALOG_H

#include <QDialog>
#include <QTimer>
#include "NGSD.h"
#include "ui_AnalysisStatusDialog.h"

//Dialog the shows the analysis status of all samples in SGE.
class AnalysisStatusDialog
	: public QDialog
{
	Q_OBJECT

public:
	AnalysisStatusDialog(QWidget *parent = 0);

protected slots:
	void analyzeSingleSamples(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void analyzeTrio(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void analyzeMultiSample(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void analyzeSomatic(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void refreshStatus();
	void showContextMenu(QPoint pos);
	void clearDetails();
	void updateDetails();
	void showOutputDetails(QTableWidgetItem* item);

private:
	Ui::AnalysisStatusDialog ui_;
	NGSD db_;
	struct JobData
	{
		int ngsd_id;
		AnalysisJob job_data;
		bool repeated;
	};
	QList<JobData> jobs_; //shown jobs only (index==row in widget)

	static void addItem(QTableWidget* table, int row, int col, QString text, QColor bg_color = Qt::transparent);
	static QColor statusToColor(QString status);
};

#endif // ANALYSISSTATUSDIALOG_H
