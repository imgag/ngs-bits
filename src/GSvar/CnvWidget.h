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
	CnvWidget(const CnvList& cnvs, QString ps_id, FilterWidget* filter_widget, ReportConfiguration& rep_conf, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent = 0);
	~CnvWidget();

protected:
	void showEvent(QShowEvent* e);

signals:
	void openRegionInIGV(QString region);
	void storeReportConfiguration();

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void applyFilters(bool debug_time=false);
	void copyToClipboard();
	void showContextMenu(QPoint p);
	void openLink(int row, int col);
	void proposeQualityIfUnset();
	void updateQuality();
	void editQuality();
	void showQcMetricHistogram();

	void updateReportConfigHeaderIcon(int row);
	void cnvHeaderDoubleClicked(int row);
	void cnvHeaderContextMenu(QPoint pos);
	void editReportConfiguration(int row);

private:
	void updateGUI();
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);
	void showSpecialTable(QString col, QString text, QByteArray url_prefix);

	Ui::CnvWidget* ui;
	QString ps_id_; //processed sample database ID. '' if unknown of NGSD is disabled.
	QString callset_id_; //CNV callset database ID. '' if unknown of if NGSD is disabled.
	const CnvList& cnvs_;
	QStringList special_cols_;
	ReportConfiguration& report_config_;
	GeneSet var_het_genes_;
	QHash<QByteArray, BedFile>& gene2region_cache_;
	bool ngsd_enabled_;
	QSet<QString> metrics_done_;
};

#endif // CNVWIDGET_H
