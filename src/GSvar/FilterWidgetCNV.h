#ifndef FILTERWIDGETCNV_H
#define FILTERWIDGETCNV_H

#include <QWidget>
#include "ui_FilterWidgetCNV.h"
#include "AnalysisDataController.h"
#include "FilterState.h"

//Filter manager dock widget
class FilterWidgetCNV
	: public QWidget
{
	Q_OBJECT
	
public:
	//Default constructor
	FilterWidgetCNV(QWidget* parent = 0);

	//Resets to initial state (uncheck boxes, no ROI)
	void reset(bool clear_roi);

	//Visually marks filters that failed.
	void markFailedFilters();

	//Sets the target region by name file (without the type prefix).
	void setTargetRegionByDisplayName(QString name);

signals:
	/// Signal that requests the creation of gene overlap ToolTips
	void calculateGeneTargetRegionOverlap();

protected slots:
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

	void editPhenotypes();
	void showPhenotypeContextMenu(QPoint pos);
	void showRoiContextMenu(QPoint pos);
	void importHPO();
	void importROI();
	void importRegion();
	void importGene();
	void importText();
	void customFilterLoaded();
	void setFilter(int index);
	void clearTargetRegion();

private:
	//Loads filters
	void loadFilters();

	//Resets the filters without blocking signals.
	void resetSignalsUnblocked(bool clear_roi);

	//Returns the filter INI file name
	QString filterFileName() const;

	Ui::FilterWidgetCNV ui_;
	AnalysisDataController& data_controller_;
	FilterState& state_;
};

#endif // FILTERWIDGETCNV_H
