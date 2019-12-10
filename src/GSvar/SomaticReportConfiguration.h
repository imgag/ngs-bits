#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H

#include "CnvList.h"
#include <QBitArray>
#include <QWidget>
#include <QDialog>
#include <QTableWidgetItem>
#include "NGSD.h"

namespace Ui {
class SomaticReportConfiguration;
}

class SomaticReportConfiguration : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticReportConfiguration(const CnvList& cnv_input, QWidget *parent = 0);
	~SomaticReportConfiguration();

	///Returns the selected CNVs
	CnvList getSelectedCNVs();

	enum report_type
	{
		DNA,
		RNA
	};

	///Returns whether report type shall be DNA or RNA
	report_type getReportType();
	///Enables radio buttons which enable the user to choose between RNA and DNA report
	void setSelectionRnaDna(bool enabled = false);

private:
	Ui::SomaticReportConfiguration *ui_;

	///List with cnvs
	CnvList cnvs_;
	///gene filter for preselection of cnvs
	GeneSet keep_genes_cnv_;

	///Filter for CNVs that are shown in widget
	QBitArray view_pass_filter;

	///NGSD neccessary for auto completion RNA file
	NGSD db_;

	///Fills CNV widget according CNV input
	void fillCnvWidget();
	///Disable all GUI elements (in case of missing CNVs)
	void disableGUI();

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
