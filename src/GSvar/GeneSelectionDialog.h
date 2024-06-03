#ifndef GENESELECTIONDIALOG_H
#define GENESELECTIONDIALOG_H

#include "FilterWidget.h"
#include "GeneSet.h"
#include "NGSHelper.h"
#include "PhenotypeList.h"
#include <QDialog>


namespace Ui {
class GeneSelectionDialog;
}

class GeneSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GeneSelectionDialog(QWidget *parent = 0);
	~GeneSelectionDialog();

	GeneSet geneSet();

protected slots:
	void phenotypesChanged();
	void editPhenotypes();
	void setPhenotypes(const PhenotypeList& phenotypes);
	void importHPO();
	void importROI();
	void determineGenes();


private:
	Ui::GeneSelectionDialog *ui_;
	GeneSet gene_set_;

	TargetRegionInfo roi_;
	GeneSet last_genes_;
	PhenotypeList phenotypes_;
	FilterWidget* variant_filter_widget_;

	void loadTargetRegions();

};

#endif // GENESELECTIONDIALOG_H
