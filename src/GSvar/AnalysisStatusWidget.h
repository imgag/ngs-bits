#ifndef AnalysisStatusWidget_H
#define AnalysisStatusWidget_H

#include <QWidget>
#include <QTimer>
#include "NGSD.h"
#include "ui_AnalysisStatusWidget.h"

//Dialog the shows the analysis status of all samples in SGE.
class AnalysisStatusWidget
	: public QWidget
{
	Q_OBJECT

public:
	AnalysisStatusWidget(QWidget *parent = 0);

signals:
	void loadFile(QString gsvar_file);

protected slots:
	void analyzeSingleSamples(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void analyzeTrio(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void analyzeMultiSample(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void analyzeSomatic(QList<AnalysisJobSample> samples=QList<AnalysisJobSample>());
	void refreshStatus();
	void showContextMenu(QPoint pos);
	void clearDetails();
	void updateDetails();
	void copyToClipboard();
	void applyTextFilter();
	void openProcessedSampleTab(int col, int row);

private:
	Ui::AnalysisStatusWidget ui_;
	struct JobData
	{
		int ngsd_id;
		AnalysisJob job_data;
		bool repeated;
	};
	QList<JobData> jobs_; //shown jobs only (index==row in widget)

	static QTableWidgetItem* addItem(QTableWidget* table, int row, int col, QString text, QColor bg_color = Qt::transparent);
	static QColor statusToColor(QString status);
	static QString timeHumanReadable(int sec);
	QList<int> selectedRows() const;
};

#endif // AnalysisStatusWidget_H
