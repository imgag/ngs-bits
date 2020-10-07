#ifndef CFDNAPANELDESIGNDIALOG_H
#define CFDNAPANELDESIGNDIALOG_H

#include <QDialog>
#include "SomaticReportConfiguration.h"
#include "VariantList.h"
#include "VcfFile.h"
#include "Settings.h"
#include "Exceptions.h"
#include "DBComboBox.h"

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
	explicit CfDNAPanelDesignDialog(const VariantList& variants, const SomaticReportConfiguration& somatic_report_configuration, const QString& processed_sample_name, const DBTable& processing_systems, QWidget *parent = 0);
	~CfDNAPanelDesignDialog();

protected slots:
	void showVariantContextMenu(QPoint pos);
	void showGeneContextMenu(QPoint pos);

private slots:
	void createOutputFiles();
	void selectAllVariants(bool deselect=false);
	void selectAllGenes(bool deselect=false);

private:
	void loadVariants();
	void loadGenes();
	Ui::CfDNAPanelDesignDialog *ui_;
	const VariantList& variants_;
	const SomaticReportConfiguration& somatic_report_configuration_;
	QString processed_sample_name_;
	QList<GeneEntry> genes_;
};

#endif // CFDNAPANELDESIGNDIALOG_H
