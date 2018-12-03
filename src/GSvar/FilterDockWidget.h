#ifndef FILTERDOCKWIDGET_H
#define FILTERDOCKWIDGET_H

#include <QDockWidget>
#include "ui_FilterDockWidget.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "Phenotype.h"
#include "FilterCascade.h"

//Filter manager dock widget
class FilterDockWidget
	: public QDockWidget
{
	Q_OBJECT
	
public:
	/// Default constructor
	FilterDockWidget(QWidget *parent = 0);
	/// Set entries of the 'filter' column valid in the open file
	void setValidFilterEntries(const QStringList& filter_entries);

	/// Resets to initial state (uncheck boxes, no ROI)
	void reset(bool clear_roi);

	/// Returns the used filters
	const FilterCascade& filters() const;
	/// Applies a filter set
	void setFilters(const QString& name, const FilterCascade& filter);
	/// Visually marks filters that failed.
	void markFailedFilters();

	///Returns the target region BED file name or an empty string if unset.
	QString targetRegion() const;
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

	/// Returns the reference sample name or an empty string if unset.
	QString referenceSample() const;

	/// Loads filter target regions (Processing systems from NGSD, Sub-panels from file system and additional target regions from INI file)
	void loadTargetRegions();

	/// Returns the row index of the currently selected filter, or -1 if none is selected;
	int currentFilterIndex() const;

signals:
	/// Signal that is emitted when a filter changes (filter cascade, gene, text, region, phenotype)
	void filtersChanged();
	/// Signal that is emitted when the filter cascade changed. Note: triggers a filtersChanged() signal as well!
	void filterCascadeChanged();
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
	void addRef();
	void removeRef();
	void referenceSampleChanged(int index);
	void geneChanged();
	void textChanged();
	void regionChanged();
	void phenotypesChanged();
	void onFilterCascadeChange(bool update_name);
	void showTargetRegionDetails();
	void updateGeneWarning();
	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);
	void updateGUI();
	void filterSelectionChanged();

	void addFilter();
	void editSelectedFilter();
	void deleteSelectedFilter();
	void moveUpSelectedFilter();
	void moveDownSelectedFilter();
	void toggleSelectedFilter(QListWidgetItem* item);

private:
	//Loads the reference file list of IGV
	void loadReferenceFiles();

	//Resets the filters without blocking signals.
	void resetSignalsUnblocked(bool clear_roi);

	//Sets the focus to the given indes (and handles border cases)
	void focusFilter(int index);

	Ui::FilterDockWidget ui_;
	GeneSet last_genes_;
	FilterCascade filters_;
	QList<Phenotype> phenotypes_;
	QStringList valid_filter_entries_;
};

#endif // FILTERDOCKWIDGET_H
