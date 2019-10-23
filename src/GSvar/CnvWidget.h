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
	CnvWidget(const CnvList& cnvs, QString ps_id, FilterWidget* filter_widget, ReportConfiguration& rep_conf, const GeneSet& het_hit_genes, QHash<QByteArray, BedFile>& cache, QWidget* parent = 0)
		: QWidget(parent)
		, ui(new Ui::CnvWidget)
		, ps_id_(ps_id)
		, cnvs_(cnvs)
		, special_cols_()
		, report_config_(rep_conf)
		, var_het_genes_(het_hit_genes)
		, gene2region_cache_(cache)
		, ngsd_enabled_(Settings::boolean("NGSD_enabled", true))
	{
		ui->setupUi(this);
		connect(ui->cnvs, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(cnvDoubleClicked(QTableWidgetItem*)));
		connect(ui->cnvs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
		connect(ui->copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
		connect(ui->filter_widget, SIGNAL(filtersChanged()), this, SLOT(applyFilters()));
		connect(ui->cnvs->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(cnvHeaderDoubleClicked(int)));
		ui->cnvs->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(ui->cnvs->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(cnvHeaderContextMenu(QPoint)));

		//set small variant filters
		ui->filter_widget->setVariantFilterWidget(filter_widget);

		//set up NGSD menu (before loading CNV - QC actions are inserted then)
		ui->ngsd_btn->setMenu(new QMenu());
		ui->ngsd_btn->menu()->addAction(QIcon(":/Icons/Edit.png"), "Edit quality", this, SLOT(editQuality()));
		ui->ngsd_btn->menu()->addSeparator();
		ui->ngsd_btn->setEnabled(ps_id_!="");

		try
		{
			updateGUI();
		}
		catch(Exception e)
		{
			addInfoLine("<font color='red'>Error parsing file:\n" + e.message() + "</font>");
			disableGUI();
		}

		//quality
		updateQuality();

		//apply filters
		applyFilters();
	}
	~CnvWidget();

signals:
	void openRegionInIGV(QString region);

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void applyFilters(bool debug_time=false);
	void copyToClipboard();
	void showContextMenu(QPoint p);
	void openLink(int row, int col);
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
	QTableWidgetItem* createItem(QString text, int alignment = Qt::AlignLeft|Qt::AlignTop);

	Ui::CnvWidget* ui;
	QString ps_id_; //processed sample database ID. '' if unknown of NGSD is disabled.
	const CnvList& cnvs_;
	QStringList special_cols_;
	ReportConfiguration& report_config_;
	GeneSet var_het_genes_;
	QHash<QByteArray, BedFile>& gene2region_cache_;
	bool ngsd_enabled_;
};

#endif // CNVWIDGET_H
