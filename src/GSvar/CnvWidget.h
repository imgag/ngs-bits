#ifndef CNVWIDGET_H
#define CNVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "CnvList.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "FilterDockWidget.h"
#include "VariantList.h"

namespace Ui {
class CnvWidget;
}

///Widget for visualization and filtering of CNVs.
class CnvWidget
	: public QWidget
{
	Q_OBJECT

public:
	CnvWidget(QString ps_filename, FilterDockWidget* filter_widget, const GeneSet& het_hit_genes, QWidget *parent = 0);
	~CnvWidget();

signals:
	void openRegionInIGV(QString region);

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void applyFilters();
	void copyToClipboard();
	void showContextMenu(QPoint p);
	void openLink(int row, int col);

private:
	void loadCNVs(QString filename);
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);
	void showSpecialTable(QString col, QString text, QByteArray url_prefix);
	QTableWidgetItem* createItem(QString text, int alignment = Qt::AlignLeft|Qt::AlignTop);

	Ui::CnvWidget* ui;
	CnvList cnvs;
	QStringList special_cols_;
	FilterDockWidget* var_filters;
	GeneSet var_het_hit_genes;
};

#endif // CNVWIDGET_H


/*
TODO:
- FIXED WIDTH FILTERS!!!
- BUTTONS for import from main window!!!
- Special filters: z-score, CN, Max freq, comp-het
- Add settings for UCSC (see paper)
- Add default filters based on CNV list type
- Test all types: CnvHunter somatic, CnvHunter multi, ClinVar germline, ClinVar somatic, ClinVar multi
*/
