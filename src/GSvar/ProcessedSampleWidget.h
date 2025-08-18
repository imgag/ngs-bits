#ifndef PROCESSEDSAMPLEWIDGET_H
#define PROCESSEDSAMPLEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QMenu>
#include "NGSD.h"
#include "DelayedInitializationTimer.h"
#include "TabBaseClass.h"

namespace Ui {
class ProcessedSampleWidget;
}

class ProcessedSampleWidget
	: public TabBaseClass
{
	Q_OBJECT


public:
	ProcessedSampleWidget(QWidget* parent, QString ps_id);
	~ProcessedSampleWidget();

	static void styleQualityLabel(QLabel* label, const QString& quality);

public slots:
    ///Loads information needed for the GUI
    void delayedInitialization();

signals:
	void clearMainTableSomReport(QString ps_name);
	void addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize);

protected slots:
	void updateGUI();
	void updateQCMetrics();
	void showPlot();
	void openSampleFolder();
	void openSampleTab();
	void openExternalDiseaseDatabase();
	void addRelation();
	void removeRelation();
	void editStudy();
	void addStudy();
	void removeStudy();
	void deleteSampleData();
	void loadVariantList();
	void queueSampleAnalysis();
	void showAnalysisInfo();

	void openIgvTrack();
	void somRepDeleted();

	void openProcessedSampleTab(QString ps);
	void openRunTab(QString name);
	void openProjectTab(QString project_name);
	void openProcessingSystemTab(QString system_short_name);

	//RNA menu
	void openGeneExpressionWidget();
	void openExonExpressionWidget();
	void openSplicingWidget();
	void openFusionWidget();

	///Opens the processed sample edit dialog
	void edit();
	///Opens sample edit dialog
	void editSample();
	///Opens a dialog to edit the diagnostic status.
	void editDiagnosticStatus();
	///Opens a dialog to edit the disease details.
	void editDiseaseDetails();
	///Opent a dialog to import data from GenLab
	void genLabImportDialog();
	///Adds missing 'same patient' relations
	void addMissingRelations(NGSD& db, QString s_id);

private:
	Ui::ProcessedSampleWidget* ui_;
    DelayedInitializationTimer init_timer_;
	QString ps_id_;

	QString sampleName() const;
	QString processedSampleName() const;
	QString mergedSamples() const;
	QStringList limitedQCParameter(const QString& sample_type);
	void addIgvMenuEntry(QMenu* menu, PathType file_type);
};

#endif // PROCESSEDSAMPLEWIDGET_H
