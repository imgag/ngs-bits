#ifndef SmallVariantSearchWidget_H
#define SmallVariantSearchWidget_H

#include <QWidget>
#include "ui_SmallVariantSearchWidget.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include "VariantList.h"

class SmallVariantSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	SmallVariantSearchWidget(QWidget* parent = 0);
	void setGene(const QString& gene);

private slots:
	void changeSearchType();
	void updateVariants();
	void copyToClipboard();
	void variantContextMenu(QPoint pos);

private:
	Ui::SmallVariantSearchWidget ui_;

	//Returns the index of the colum with the given name. Throws an exception, it it is not found.
	int columnIndex(QString name) const;
	//Variant search
	void getVariantsForRegion(Chromosome chr, int start, int end, QByteArray gene, const GeneSet& gene_symbols, QList<QList<QVariant>>& output, QStringList& messages);

};

#endif // SmallVariantSearchWidget_H
