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
	CnvWidget(QWidget* parent, const CnvList& cnvs, QString ps_id, QSharedPointer<ReportConfiguration> rep_conf, QSharedPointer<SomaticReportConfiguration> rep_conf_somatic, const GeneSet& het_hit_genes);
	~CnvWidget();

protected:
	void showEvent(QShowEvent* e);

signals:
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

	///Flags all filtered (=currently invisible) CNVs as artefacts in somatic report configuration
	void flagInvisibleSomaticCnvsAsArtefacts();
	///Flags all unfiltered (=currently visible) CNVs as artefacts in somatic report configuration
	void flagVisibleSomaticCnvsAsArtefacts();

	void openColumnSettings();
	void adaptColumnWidthsAndHeights();
	void showAllColumns();

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
	///Upload CNV to ClinVar
	void uploadToClinvar(int index1, int index2=-1);

	Ui::CnvWidget* ui;
	QString ps_id_; //processed sample database ID. '' if unknown or if NGSD is disabled.
	QString callset_id_; //CNV callset database ID. '' if unknown or if NGSD is disabled.
	const CnvList& cnvs_;
	QStringList special_cols_;
	QSharedPointer<ReportConfiguration> report_config_;
	QSharedPointer<SomaticReportConfiguration> somatic_report_config_;

	GeneSet var_het_genes_;
	QSet<QString> metrics_done_;
    bool ngsd_user_logged_in_;
	bool rc_enabled_;
	bool is_somatic_ = false;
};

#endif // CNVWIDGET_H
