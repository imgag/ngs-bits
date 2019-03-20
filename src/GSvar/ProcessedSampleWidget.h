#ifndef PROCESSEDSAMPLEWIDGET_H
#define PROCESSEDSAMPLEWIDGET_H

#include "NGSD.h"

#include <QWidget>
#include <QLabel>

namespace Ui {
class ProcessedSampleWidget;
}

class ProcessedSampleWidget
	: public QWidget
{
	Q_OBJECT


public:
	ProcessedSampleWidget(QWidget* parent, QString ps_id);
	~ProcessedSampleWidget();

	static void styleQualityLabel(QLabel* label, const QString& quality);

signals:
	void openProcessedSampleTab(QString ps_name);
	void openRunTab(QString run_name);
	void executeIGVCommands(QStringList commands);

protected slots:
	void updateGUI();
	void updateQCMetrics();
	void showPlot();
	void openSampleInNGSD();
	void openSampleFolder();
	void openSampleTab();

	void addBamToIgv();
	void addCnvsToIgv();

	///Opens a dialog to edit the diagnostic status.
	void editDiagnosticStatus();
	///Opens a dialog to edit the disease group/info.
	void editDiseaseGroupAndInfo();
	///Opens a dialog to edit the disease details.
	void editDiseaseDetails();
	///Sets the processed sample quality.
	void setQuality();

private:
	Ui::ProcessedSampleWidget* ui_;
	QString ps_id_;
	NGSD db_;

	QString processedSampleName() const;
};

#endif // PROCESSEDSAMPLEWIDGET_H
