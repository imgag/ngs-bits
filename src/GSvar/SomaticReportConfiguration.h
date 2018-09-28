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

	///Returns the selected CNVs
	ClinCnvList getSelectedCNVs();

private:
	Ui::SomaticReportConfiguration *ui_;

	///List with cnvs
	ClinCnvList cnvs_;
	///gene filter for preselection of cnvs
	GeneSet keep_genes_cnv_;

private slots:
	void showContextMenu(QPoint pos);
};

#endif // SOMATICREPORTCONFIGURATION_H
