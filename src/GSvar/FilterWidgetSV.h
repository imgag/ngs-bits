#ifndef FILTERWIDGETSV_H
#define FILTERWIDGETSV_H

#include <QWidget>
#include "ui_FilterWidgetSV.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "Phenotype.h"
#include "FilterWidget.h"

//Filter manager dock widget
class FilterWidgetSV
	: public QWidget
{
	Q_OBJECT

public:
	////Default constructor
	FilterWidgetSV(QWidget* parent = 0);
	////Sets the small variant filter widget (needed to import data from it)
	void setVariantFilterWidget(FilterWidget* filter_widget);

	/// Set entries of the 'filter' column valid in the open file
	void setValidFilterEntries(const QStringList& filter_entries);

	////Resets to initial state (uncheck boxes, no ROI)
	void reset(bool clear_roi);

	////Returns the used filters
	const FilterCascade& filters() const;
	////Visually marks filters that failed.
	void markFailedFilters();

	////Returns the target region BED file name or an empty string if unset.
	QString targetRegion() const;
	////Sets the target region BED file.
	void setTargetRegion(QString roi_file);

	////Returns the gene names filter.
	GeneSet genes() const;
	////Returns the text filter.
	QByteArray text() const;
	////Returns the single target region filter, or an empty string if unset.
	QString region() const;
	////Sets the single target region filter, or an empty string if unset.
	void setRegion(QString region);

	////Returns selected phenotype terms.
	const QList<Phenotype>& phenotypes() const;
	////Sets selected phenotype terms.
	void setPhenotypes(const QList<Phenotype>& phenotypes);


signals:
	////Signal that is emitted when a filter changes (filter cascade, gene, text, region, phenotype)
	void filtersChanged();
	////Signal is emitted when the target region changes
	void targetRegionChanged();

protected slots:
	void roiSelectionChanged(int index);
	void geneChanged();
	void textChanged();
	void regionChanged();
	void phenotypesChanged();
	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);
	void importHPO();
	void importROI();
	void importRegion();
	void importGene();
	void importText();
	void updateFilterName();
	void setFilter(int index);
	void clearTargetRegion();

private:
	////Loads filters
	void loadFilters();

	////Resets the filters without blocking signals.
	void resetSignalsUnblocked(bool clear_roi);

	////Returns the filter INI file name
	QString filterFileName() const;

	Ui::FilterWidgetSV ui_;
	GeneSet last_genes_;
	QList<Phenotype> phenotypes_;
	FilterWidget* filter_widget_;
};

#endif // FILTERWIDGETSV_H
