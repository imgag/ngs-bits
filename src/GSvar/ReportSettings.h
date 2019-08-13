#ifndef REPORTSETTINGS_H
#define REPORTSETTINGS_H

#include "VariantType.h"
#include "NGSD.h"
#include <QStringList>

///Variant meta data for report.
struct ReportVariantConfiguration
{
	///Default constructor
	ReportVariantConfiguration();
	///Returns if the variant is to be shown in the report
	bool showInReport() const;

	VariantType variant_type;
	int variant_index;

	QString type;
	bool de_novo;
	QString inheritance_mode;

	static QStringList getTypeOptions();
	static QStringList getInheritanceModeOptions();
};

///Report meta data.
struct ReportSettings
{
	///Default constructor
	ReportSettings();
	///Returns the indices of variants
	QList<int> variantIndices(VariantType type, bool only_selected) const;

	DiagnosticStatusData diag_status; //diagnostic status

	QList<ReportVariantConfiguration> variant_config; //variant configuration

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
