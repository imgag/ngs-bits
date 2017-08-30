#ifndef CNVWIDGET_H
#define CNVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "CnvList.h"
#include "BedFile.h"
#include "GeneSet.h"

namespace Ui {
class CnvList;
}

///Widget for visualization and filtering of CNVs.
class CnvWidget
	: public QWidget
{
	Q_OBJECT

public:
	explicit CnvWidget(QString filename, QWidget *parent = 0);
	~CnvWidget();

	void setGenesFilter(const GeneSet& genes);
	void setRoiFilter(QString filename);

signals:
	void openRegionInIGV(QString region);

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void filtersChanged();
	void copyToClipboard();
	void annotationFilterColumnChanged();
	void annotationFilterOperationChanged();
	void showContextMenu(QPoint p);

private:
	void loadCNVs(QString filename);
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);

	Ui::CnvList *ui;
	CnvList cnvs;
	GeneSet f_genes;
	BedFile f_roi;
	QMap<QString, int> annotation_col_indices_;
};

#endif // CNVWIDGET_H
