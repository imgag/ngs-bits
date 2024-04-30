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
	void executeAction();
	void deleteUnusedSamples();
	void deleteUnusedVariants();
	void importStudySamples();
	void replaceObsolteHPOTerms();
	void findInconsistenciesForCausalDiagnosticVariants();
	void importYearOfBirth();
	void importTissue();
	void importPatientIDs();
	void linkSamplesFromSamePatient();
	void deleteVariantsOfBadSamples();

private:
	Ui::MaintenanceDialog ui_;

	void appendOutputLine(QString line);
	QSet<QString> tablesReferencing(NGSD& db, QString referenced_table);
};

#endif // MAINTENANCEDIALOG_H
