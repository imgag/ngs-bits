#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>
#include "ui_FilterWidget.h"
#include "GeneSet.h"
#include "PhenotypeList.h"
#include "FilterCascade.h"
#include "NGSHelper.h"

//Filter settings for report configuration
enum class ReportConfigFilter
{
	NONE,
	NO_RC,
	HAS_RC
};

//Filter manager dock widget
//The filter list is not populated automatically, since we need to get the database
//credentials first. As soon as the user logs in, the app calls loadTargetRegions()
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

	///Returns the target region BED file. Empty if unset.
	const TargetRegionInfo& targetRegion() const;
	///Returns the target region display name or an empty string if unset.
	QString targetRegionDisplayName() const;
	///Sets the target region by name file (without the type prefix). Returns if the target region name was found and set.
	bool setTargetRegionByDisplayName(QString name);

	/// Returns the gene names filter.
	GeneSet genes() const;
	/// Returns the text filter.
	QByteArray text() const;
	/// Returns the single target region filter, or an empty string if unset.
	QString region() const;
	/// Sets the single target region filter, or an empty string if unset.
	void setRegion(QString region);
	/// Returns the state of the report configuration
	ReportConfigFilter reportConfigurationFilter() const;
	/// Disables checkbox for option of reportConfigurationVariantsOnly
	void disableReportConfigurationFilter() const;

	///Returns selected phenotype terms.
	const PhenotypeList& phenotypes() const;
	///Sets selected phenotype terms.
	void setPhenotypes(const PhenotypeList& phenotypes);

	///Returns selected phenotype terms.
	const PhenotypeSettings& phenotypeSettings() const;
	///Sets selected phenotype terms.
	void setPhenotypeSettings(const PhenotypeSettings& settings);

	/// Loads filter target regions (Processing systems from NGSD, Sub-panels from file system and additional target regions from INI file)
	void loadTargetRegions();
	/// Helper for loading target regions (also in CNV/SV widget)
	static void loadTargetRegions(QComboBox* box);
	/// Helper for loading target region data. Throws an exception of the target region file is missing!
	static void loadTargetRegionData(TargetRegionInfo& roi, QString name);
	/// Helper for checking that gene names are approved symbols (also in CNV/SV widget)
	static void checkGeneNames(const GeneSet& genes, QLineEdit* widget);

	///Returns the filter INI file name
	static QString filterFileName();
	///Sets the filter by name. Returns if the filter name was found and set.
	bool setFilter(QString name);
	///Sets filterCascade
	void setFilterCascade(const FilterCascade& filter_cascade);
	void editColumnFilter(QString col);

	///Returns current filter name
	QString filterName() const;

	//Updates widgets according to NGSD support
	void updateNGSDSupport();

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
	void phenotypesChanged();
	void addRoi();
	void addRoiTemp();
	void removeRoi();
	void roiSelectionChanged(int index);
	void geneChanged();
	void textChanged();
	void regionChanged();
	void reportConfigFilterChanged();
	void updateFilterName();
	void customFilterLoaded();
	void showTargetRegionDetails();
	void copyTargetRegionToClipboard();
	void copyGenesToClipboard();
	void openTargetRegionInIGV();
	void updateGeneWarning();
	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);
	void showGeneContextMenu(QPoint pos);
	void setFilter(int index);
	void clearTargetRegion();
	void clearFilters();
	void clearFiltersAndRoi();

private:
	//Loads filters
	void loadFilters();
	//Resets the filters without blocking signals.
	void resetSignalsUnblocked(bool clear_roi);

	Ui::FilterWidget ui_;
	TargetRegionInfo roi_;
	GeneSet last_genes_;
	PhenotypeList phenotypes_;
	PhenotypeSettings phenotype_settings_;
};

#endif // FILTERWIDGET_H
