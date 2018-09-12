#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H

#include "ClinCnvList.h"
#include <QWidget>
#include <QDialog>

namespace Ui {
class SomaticReportConfiguration;
}

class SomaticReportConfiguration : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticReportConfiguration(const ClinCnvList& cnv_input, GeneSet keep_genes, QWidget *parent = 0);
	~SomaticReportConfiguration();

	///returns CnvList with cnvs which have an activated checkbox
	ClinCnvList getFilteredVariants();

private:
	Ui::SomaticReportConfiguration *ui_;

	///List with cnvs
	ClinCnvList cnvs_;
	///gene filter for preselection of cnvs
	GeneSet keep_genes_cnv_;

	///returns true if variants should be kept, false otherwise
//	bool preselectVariant(const CopyNumberVariant& variant);

private slots:
	void showContextMenu(QPoint pos);
};

#endif // SOMATICREPORTCONFIGURATION_H
