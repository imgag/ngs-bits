#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

#include <QWidget>
#include "ui_FilterWidget.h"
#include "GeneSet.h"
#include "PhenotypeList.h"
#include "FilterCascade.h"
#include "NGSHelper.h"
#include "FilterState.h"
#include "AnalysisDataController.h"

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

	/// Visually marks filters that failed.
	void markFailedFilters();

	///Returns the target region display name or an empty string if unset.
	QString targetRegionDisplayName() const;
    QComboBox* targetRegionBox();
    /// Loads filter target regions (Processing systems from NGSD, Sub-panels from file system and additional target regions from INI file)
	void loadTargetRegions();


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

public slots:
    ///Sets the target region by name file (without the type prefix). Returns if the target region name was found and set.
    bool setTargetRegionByName(QString name);

    void updateFilterName();
    void updateFilterCascade();
    void updateTargetRegionFilter(int index);
    void updateTargetRegionFilter(const TargetRegionInfo& new_target);
    void updateRegionFilter();
    void updatePhenotypes();
    void updateGeneFilter();
    void updateTextFilter();
    void updateReportConfigfilter();

	void phenotypesChanged();

	void addRoi();
	void addRoiTemp();
	void removeRoi();

	void customFilterLoaded();
	void showTargetRegionDetails();
	void copyTargetRegionToClipboard();
	void copyGenesToClipboard();
	void openTargetRegionInIGV();
	void updateGeneWarning();
	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);
	void showGeneContextMenu(QPoint pos);
	void showRoiContextMenu(QPoint pos);
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
    AnalysisDataController& data_controller_;
    FilterState& state_;
};

#endif // FILTERWIDGET_H
