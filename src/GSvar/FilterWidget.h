#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>
#include "ui_FilterWidget.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "Phenotype.h"
#include "FilterCascade.h"

//Filter manager dock widget
class FilterWidget
	: public QWidget
{
	Q_OBJECT
	
public:
	/// Default constructor
	FilterWidget(QWidget *parent = 0);
	/// Set entries of the 'filter' column valid in the open file
	void setValidFilterEntries(const QStringList& filter_entries);

	/// Resets to initial state (uncheck boxes, no ROI)
	void reset(bool clear_roi);

	/// Returns the used filters
	const FilterCascade& filters() const;
	/// Visually marks filters that failed.
	void markFailedFilters();

	///Returns the target region BED file or an empty string if unset.
	QString targetRegion() const;
	///Returns the target region display name or an empty string if unset.
	QString targetRegionName() const;
	///Sets the target region BED file.
	void setTargetRegion(QString roi_file);

	/// Returns the gene names filter.
	GeneSet genes() const;
	/// Returns the text filter.
	QByteArray text() const;
	/// Returns the single target region filter, or an empty string if unset.
	QString region() const;
	/// Sets the single target region filter, or an empty string if unset.
	void setRegion(QString region);

	///Returns selected phenotype terms.
	const QList<Phenotype>& phenotypes() const;
	///Sets selected phenotype terms.
	void setPhenotypes(const QList<Phenotype>& phenotypes);

	/// Loads filter target regions (Processing systems from NGSD, Sub-panels from file system and additional target regions from INI file)
	void loadTargetRegions();

signals:
	/// Signal that is emitted when a filter changes (filter cascade, gene, text, region, phenotype)
	void filtersChanged();
	/// Signal is emitted when the target region changes
	void targetRegionChanged();
	/// Signal that loading phenotype data from NGSD was requested (this cannot be done inside the widget, because it knows nothing about the sample)
	void phenotypeImportNGSDRequested();
	/// Signal that a sub-panel should be created using the phenotypes
	void phenotypeSubPanelRequested();

protected slots:
	void addRoi();
	void addRoiTemp();
	void removeRoi();
	void roiSelectionChanged(int index);
	void geneChanged();
	void textChanged();
	void regionChanged();
	void phenotypesChanged();
	void updateFilterName();
	void showTargetRegionDetails();
	void updateGeneWarning();
	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);
	void setFilter(int index);
	void clearTargetRegion();

private:
	//Loads filters
	void loadFilters();
	//Resets the filters without blocking signals.
	void resetSignalsUnblocked(bool clear_roi);

	//Returns the filter INI file name
	QString filterFileName() const;

	Ui::FilterWidget ui_;
	GeneSet last_genes_;
	QList<Phenotype> phenotypes_;
};

#endif // FILTERWIDGET_H
