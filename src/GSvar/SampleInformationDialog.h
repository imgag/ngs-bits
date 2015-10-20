#ifndef SAMPLEINFORMATIONDIALOG_H
#define SAMPLEINFORMATIONDIALOG_H

#include <QDialog>
#include <QTimer>
#include "ui_SampleInformationDialog.h"
#include "NGSD.h"

///Report generation dialog
class SampleInformationDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	///Constructor
	SampleInformationDialog(QWidget* parent, QString filename);

private slots:
	///Starts reanalysis of the sample.
	void reanalyze();
	///Refreshes reanalysis information.
	void refreshReanalysisStatus();
	///Sets the report status according to the sender action text.
	void setReportStatus();

private:
	///Refreshes information based on filename.
	void refresh();

	///Formats a statistics label (outliers are colored red).
	void statisticsLabel(QLabel* label, QString accession, const QCCollection& qc);

	Ui::SampleInformationDialog ui_;
	NGSD db_;
	QString filename_;
	QString reanalyze_status_;
	QTimer timer_;
};

#endif
