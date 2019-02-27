#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H

#include "ClinCnvList.h"
#include <QBitArray>
#include <QWidget>
#include <QDialog>
#include <QTableWidgetItem>

namespace Ui {
class SomaticReportConfiguration;
}

class SomaticReportConfiguration : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticReportConfiguration(const ClinCnvList& cnv_input, QWidget *parent = 0);
	~SomaticReportConfiguration();

	///Returns the selected CNVs
	ClinCnvList getSelectedCNVs();

private:
	Ui::SomaticReportConfiguration *ui_;

	///List with cnvs
	ClinCnvList cnvs_;
	///gene filter for preselection of cnvs
	GeneSet keep_genes_cnv_;

	///Filter for CNVs that are shown in widget
	QBitArray view_pass_filter;

private slots:
	void showContextMenu(QPoint pos);
	///update widget containing CNVs after filters were adjusted
	void filtersChanged();

	///Resets view filters
	void resetView();

	///Selects variants according to view filter
	void selectCNVsFromView();
	///Deselects all gonosomes
	void deselectXY();
	///Emits signal openRegionInIGV if double click on CNV
	void cnvDoubleClicked(QTableWidgetItem* item);
	///Copies selected rows from cnvs-widget to clipboard
	void copyToClipboard();


signals:
	void openRegionInIGV(QString region);
};

#endif // SOMATICREPORTCONFIGURATION_H
