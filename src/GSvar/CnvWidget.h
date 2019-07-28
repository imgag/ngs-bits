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

private:
	void loadCNVs(QString filename);
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);
	QTableWidgetItem* createItem(QString text, int alignment = Qt::AlignLeft|Qt::AlignTop);

	Ui::CnvWidget* ui;
	CnvList cnvs;
	FilterDockWidget* var_filters;
	GeneSet var_het_hit_genes;
};

#endif // CNVWIDGET_H


/*
TODO:
- Links to webpages, e.g. ClinGen https://www.ncbi.nlm.nih.gov/projects/dbvar/clingen/clingen_gene.cgi?sym=BRCA1
- Special filters: z-score, CN, Max freq, comp-het
- Add settings for UCSC (see paper)
- Split special annoation for tool-tip
- Implement all types: CnvHunter somatic, CnvHunter multi, ClinVar germline, ClinVar somatic, ClinVar multi
- Add default filters based on CNV list type
*/
