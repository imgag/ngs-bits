#ifndef CFDNAPANELDESIGNDIALOG_H
#define CFDNAPANELDESIGNDIALOG_H

#include <QDialog>
#include "SomaticReportConfiguration.h"
#include "VariantList.h"
#include "Settings.h"
#include "Exceptions.h"

struct GeneEntry
{
	QString gene_name;
	Chromosome chr;
	int start;
	int end;
	QDate date;
	QString file_path;
};

namespace Ui {
class CfDNAPanelDesignDialog;
}

class CfDNAPanelDesignDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CfDNAPanelDesignDialog(const VariantList& variants, const SomaticReportConfiguration& somatic_report_configuration, const QString& processed_sample_name, const QString& system_name, QWidget *parent = 0);
	~CfDNAPanelDesignDialog();

private slots:
	void createOutputFiles();

private:
	Ui::CfDNAPanelDesignDialog *ui_;
	void loadVariants();
	void loadGenes();
	const VariantList& variants_;
	VariantList selected_variants_;
	QString processed_sample_name_;
	QString system_name_;
	const SomaticReportConfiguration& somatic_report_configuration_;
	QList<GeneEntry> genes_;
};

#endif // CFDNAPANELDESIGNDIALOG_H
