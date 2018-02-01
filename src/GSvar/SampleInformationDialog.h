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
	SampleInformationDialog(QWidget* parent, QString processed_sample_name);

private slots:
	///Starts reanalysis of the sample.
	void reanalyze();
	///Refreshes reanalysis information.
	void refreshReanalysisStatus();
	///Opens a dialog to view/edit the diagnostic status.
	void editDiagnosticStatus();
	///Sets the processed sample quality according to the sender action text.
	void setQuality();

private:
	///Refreshes information based on filename.
	void refresh();

	///Formats a statistics label (outliers are colored red).
	void statisticsLabel(NGSD& db, QLabel* label, QString accession, const QCCollection& qc, bool label_outlier_low, bool label_outlier_high, int decimal_places = 2);

	Ui::SampleInformationDialog ui_;
	QString processed_sample_name_;
	QString reanalyze_status_;
	QTimer timer_;
};

#endif
