#ifndef CNVWIDGET_H
#define CNVWIDGET_H

#include "ui_CnvWidget.h"
#include <QWidget>
#include <QTableWidgetItem>
#include <QMenu>
#include "CnvList.h"
#include "GeneSet.h"
#include "FilterWidget.h"
#include "VariantTable.h"
#include "Settings.h"

namespace Ui {
class CnvWidget;
}

///Widget for visualization and filtering of CNVs.
class CnvWidget
	: public QWidget
{
	Q_OBJECT

public:

	CnvWidget(const CnvList& cnvs, QString ps_id, FilterWidget* filter_widget, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent = 0);

	//Constructor for germline samples
	CnvWidget(const CnvList& cnvs, QString ps_id, FilterWidget* filter_widget, QSharedPointer<ReportConfiguration> rep_conf, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent = 0);
	//Constructor for tumor-normal pairs
	CnvWidget(const CnvList& cnvs, QString t_ps_id, FilterWidget* filter_widget, SomaticReportConfiguration& rep_conf, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent = 0);

	~CnvWidget();

protected:
	void showEvent(QShowEvent* e);

signals:
	void openRegionInIGV(QString region);
	void openGeneTab(QString region);
	void storeSomaticReportConfiguration();

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void applyFilters(bool debug_time=false);
	void copyToClipboard();
	void showContextMenu(QPoint p);
	void proposeQualityIfUnset();
	void updateQuality();
	void editQuality();
	void showQcMetricHistogram();

	void updateReportConfigHeaderIcon(int row);
	void cnvHeaderDoubleClicked(int row);
	void cnvHeaderContextMenu(QPoint pos);
	void editReportConfiguration(int row);	
	void importPhenotypesFromNGSD();

	///Loads the gene file to a given target region BED file
	void annotateTargetRegionGeneOverlap();
	///Removes the calculated gene overlap tooltips
	void clearTooltips();

private:
	void initGUI();
	void updateGUI();
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);
	void editGermlineReportConfiguration(int row);
	void editSomaticReportConfiguration(int row);
	///Edit validation status of current cnv
	void editCnvValidation(int row);
	///Handles somatic report configuration if multiple rows are selected;
	void editSomaticReportConfiguration(const QList<int>& rows);
	Ui::CnvWidget* ui;
	QString ps_id_; //processed sample database ID. '' if unknown of NGSD is disabled.
	QString callset_id_; //CNV callset database ID. '' if unknown of if NGSD is disabled.
	const CnvList& cnvs_;
	QStringList special_cols_;
	QSharedPointer<ReportConfiguration> report_config_;
	SomaticReportConfiguration* somatic_report_config_;
	bool is_somatic_ = false;

	GeneSet var_het_genes_;
	QHash<QByteArray, BedFile>& gene2region_cache_;
	QSet<QString> metrics_done_;
	bool ngsd_enabled_;
};

#endif // CNVWIDGET_H
