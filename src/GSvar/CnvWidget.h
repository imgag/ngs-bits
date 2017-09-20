#ifndef CNVWIDGET_H
#define CNVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "CnvList.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "FilterDockWidget.h"

namespace Ui {
class CnvList;
}

///Widget for visualization and filtering of CNVs.
class CnvWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit CnvWidget(QString filename, FilterDockWidget* filter_widget, QWidget *parent = 0);
	~CnvWidget();

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

	FilterDockWidget* var_filters;
	Ui::CnvList *ui;
	CnvList cnvs;
	QMap<QString, int> annotation_col_indices_;
};

#endif // CNVWIDGET_H
