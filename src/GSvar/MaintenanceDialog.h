#ifndef MAINTENANCEDIALOG_H
#define MAINTENANCEDIALOG_H

#include <QDialog>
#include "NGSD.h"
#include "ui_MaintenanceDialog.h"

class MaintenanceDialog: public QDialog
{
	Q_OBJECT

public:
	 MaintenanceDialog(QWidget *parent = nullptr);

private slots:
	//main actions
	void deleteUnusedSamples();
	void deleteUnusedVariants();
	void importStudySamples();
	void replaceObsolteHPOTerms();
	void replaceObsolteGeneSymbols();
	void findInconsistenciesForCausalDiagnosticVariants();
	void importYearOfBirth();
	void importTissue();
	void importPatientIDs();
	void linkSamplesFromSamePatient();
	void deleteVariantsOfBadSamples();
	void deleteDataOfMergedSamples();
	void compareStructureOfTestAndProduction();
	void compareBaseDataOfTestAndProduction();
	void findTumorSamplesWithGermlineVariants();
	void deleteInvalidVariants();

	//auxilary slots
	void updateDescription(QString text);
	void executeAction();

private:
	Ui::MaintenanceDialog ui_;

	void appendOutputLine(QString line);
	QSet<QString> tablesReferencing(NGSD& db, QString referenced_table);
	QString compareCount(NGSD& db_p, NGSD& db_t, QString query);
	void deleteVariants(NGSD& db, int ps_id, int c_deleted, int c_failed);
	QSet<int> psWithVariants(NGSD& db);
	void clearUnusedReportConfigs(NGSD& db);
	void fixGeneNames(NGSD& db, QString table, QString column);
	void deleteVariant(NGSD& db, QString var_id, QString reason);
};

#endif // MAINTENANCEDIALOG_H
