#ifndef CFDNAPANELDESIGNDIALOG_H
#define CFDNAPANELDESIGNDIALOG_H

#include <FilterCascade.h>
#include <QDialog>
#include <QTableWidgetItem>
#include "SomaticReportConfiguration.h"
#include "VariantList.h"
#include "VcfFile.h"
#include "Settings.h"
#include "Exceptions.h"
#include "DBComboBox.h"
#include "NGSD.h"

namespace Ui {
class CfDNAPanelDesignDialog;
}

class CfDNAPanelDesignDialog : public QDialog
{
	Q_OBJECT

public:
	CfDNAPanelDesignDialog(const VariantList& variants, const FilterResult& filter_result, QSharedPointer<SomaticReportConfiguration> somatic_report_configuration, const QString& processed_sample_name, const DBTable& processing_systems, QWidget *parent = 0);
	~CfDNAPanelDesignDialog();

private slots:
	void showVariantContextMenu(QPoint pos);
	void showHotspotContextMenu(QPoint pos);
	void showGeneContextMenu(QPoint pos);
	void showHotspotRegions(int state);
	void selectAllVariants(bool deselect=false);
	void selectAllHotspotRegions(bool deselect=false);
	void selectAllGenes(bool deselect=false);
	void updateSelectedVariantCount();
	void updateSelectedHotspotCount();
	void openVariantInIGV(QTableWidgetItem* item);
	void updateSystemSelection();
	void writePanelToFile();
	void storePanelInNGSD();
	void addVariant();

private:
	void loadPreviousPanels();
	void loadVariants();
	void loadGenes();
	void loadHotspotRegions();
	VcfFile createVcfFile();
	BedFile createBedFile(const VcfFile& vcf_file);
	int selectedVariantCount();

	Ui::CfDNAPanelDesignDialog *ui_;
	const VariantList& variants_;
	const FilterResult& filter_result_;
	const QSharedPointer<SomaticReportConfiguration> somatic_report_configuration_;
	QMap<QString, bool> prev_vars_;
	QMap<QString, bool> candidate_vars_;
	QMap<QString, float> candidate_scores_;
	QSet<QString> prev_genes_;
	bool prev_id_snp_;
	QMap<QString, bool> prev_hotspots_;
	QString processed_sample_name_;
    QString processed_sample_id_;
	QList<CfdnaGeneEntry> genes_;
	CfdnaPanelInfo cfdna_panel_info_;
};

#endif // CFDNAPANELDESIGNDIALOG_H
