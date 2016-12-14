#ifndef CNVWIDGET_H
#define CNVWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "CnvList.h"
#include "BedFile.h"

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

	void setGenesFilter(QStringList genes);
	void setRoiFilter(QString filename);

signals:
	void openRegionInIGV(QString region);

private slots:
	void cnvDoubleClicked(QTableWidgetItem* item);
	void filtersChanged();
	void copyToClipboard();

private:
	void loadCNVs(QString filename);
	void disableGUI();
	void addInfoLine(QString text);
	void updateStatus(int shown);

	Ui::CnvList *ui;
	CnvList cnvs;
	QStringList f_genes;
	BedFile f_roi;
};

#endif // CNVWIDGET_H
