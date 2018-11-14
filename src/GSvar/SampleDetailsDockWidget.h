#ifndef SAMPLEDETAILSDOCKWIDGET_H
#define SAMPLEDETAILSDOCKWIDGET_H

#include <QDockWidget>
#include "ui_SampleDetailsDockWidget.h"
#include "QCCollection.h"

class SampleDetailsDockWidget
	: public QDockWidget
{
	Q_OBJECT

public:
	SampleDetailsDockWidget(QWidget *parent = 0);

	///Refreshes information based on processed sample name.
	void refresh(QString processed_sample_name);

	///Clears all information;
	void clear();

private slots:
	///Opens a dialog to view/edit the diagnostic status.
	void editDiagnosticStatus();
	///Opens a dialog to view/edit the disease data.
	void editDiseaseData();
	///Sets the processed sample quality according to the sender action text.
	void setQuality();

private:
	Ui::SampleDetailsDockWidget ui_;
	QString processed_sample_name_;

	///Formats a statistics label (outliers are colored red).
	void statisticsLabel(QLabel* label, QString accession, const QCCollection& qc, bool label_outlier_low, bool label_outlier_high, int decimal_places = 2, QString suffix="");

};

#endif // SAMPLEDETAILSDOCKWIDGET_H
