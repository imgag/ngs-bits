#ifndef CLINCNVWIDGET_H
#define CLINCNVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "ClinCnvList.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "FilterDockWidget.h"
#include "VariantList.h"

namespace Ui {
	class ClinCnvWidget;
}

///Widget for visualization and filtering of CNVs.
class ClinCnvWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit ClinCnvWidget(QString filename, FilterDockWidget* filter_widget, const GeneSet& het_hit_genes, QWidget *parent = 0);
	~ClinCnvWidget();

signals:
	void openRegionInIGV(QString region);

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void filtersChanged();
	void variantFiltersChanged();
	void copyToClipboard();
	void annotationFilterColumnChanged();
	void annotationFilterOperationChanged();
	void showContextMenu(QPoint p);

private:
	void loadCNVs(QString filename);
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);

	FilterDockWidget* var_filters_;
	GeneSet var_het_hit_genes_;
	Ui::ClinCnvWidget *ui;
	ClinCnvList cnvs;
	bool is_somatic_;
};

#endif // CLINCNVWIDGET_H
