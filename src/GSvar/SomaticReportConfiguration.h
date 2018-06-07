#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H

#include "CnvList.h"
#include <QWidget>
#include <QDialog>

namespace Ui {
class SomaticReportConfiguration;
}

class SomaticReportConfiguration : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticReportConfiguration(const CnvList& cnv_input, GeneSet keep_genes, QWidget *parent = 0);
	~SomaticReportConfiguration();

	///returns CnvList with cnvs which have an activated checkbox
	CnvList getFilteredVariants();

private:
	Ui::SomaticReportConfiguration *ui_;

	///List with cnvs
	CnvList cnvs_;
	///gene filter for preselection of cnvs
	GeneSet keep_genes_cnv_;

	///returns true if variants should be kept, false otherwise
	bool preselectVariant(const CopyNumberVariant& variant);

private slots:
	void showContextMenu(QPoint pos);
};

#endif // SOMATICREPORTCONFIGURATION_H
