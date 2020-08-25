#ifndef SMALLVARIANTSEARCHDIALOG_H
#define SMALLVARIANTSEARCHDIALOG_H

#include <QDialog>
#include "ui_SmallVariantSearchDialog.h"
#include "DelayedInitializationTimer.h"
#include "Chromosome.h"
#include "GeneSet.h"

class SmallVariantSearchDialog
	: public QDialog
{
	Q_OBJECT

public:
	SmallVariantSearchDialog(QWidget* parent = 0);
	void setGene(const QString& gene);

private slots:
	void changeSearchType();
	void updateVariants();
	void copyToClipboard();

private:
	Ui::SmallVariantSearchDialog ui_;
	DelayedInitializationTimer init_timer_;

	void getVariantsForRegion(Chromosome chr, int start, int end, QByteArray gene, const GeneSet& gene_symbols, QList<QStringList>& output, QStringList& messages);

};

#endif // SMALLVARIANTSEARCHDIALOG_H
