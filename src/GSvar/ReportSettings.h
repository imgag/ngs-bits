#ifndef REPORTSETTINGS_H
#define REPORTSETTINGS_H

#include "VariantType.h"
#include "NGSD.h"
#include <QStringList>

///Report meta data.
class ReportSettings
{
public:
	///Default constructor
	ReportSettings();

	DiagnosticStatusData diag_status; //diagnostic status

	ReportConfiguration report_config; //report configuration

	bool show_coverage_details; //slow low-coverage details
	int min_depth; //cutoff for low-coverage statistics
	bool roi_low_cov; //low-coverage details for the ROI are added (not only for CCDS)
	bool recalculate_avg_depth; //average coverage should be calculated for the target region (otherwise the processing system average depth is used)
	bool show_tool_details; //show tool version and parameter table
	bool show_omim_table; //show OMIM table
	bool show_class_details; //show classification information
	QString language;

};


#endif // REPORTSETTINGS_H
