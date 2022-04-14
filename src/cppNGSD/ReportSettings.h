#ifndef REPORTSETTINGS_H
#define REPORTSETTINGS_H

#include "cppNGSD_global.h"
#include "VariantType.h"
#include "ReportConfiguration.h"
#include <QStringList>

///Report meta data.
class CPPNGSDSHARED_EXPORT ReportSettings
{
public:
	///Default constructor
	ReportSettings();

	QSharedPointer<ReportConfiguration> report_config; //report configuration
	QString report_type;
	QList<QPair<VariantType, int>> selected_variants;
	bool select_other_causal_variant = false;

	bool show_coverage_details; //slow low-coverage details
	int min_depth; //cutoff for low-coverage statistics
	bool roi_low_cov; //low-coverage details for the ROI are added (not only for CCDS)
	bool recalculate_avg_depth; //average coverage should be calculated for the target region (otherwise the processing system average depth is used)
	bool show_omim_table; //show OMIM table
	bool show_one_entry_in_omim_table; //show only one phenotype entry per gene in OMIM table
	bool show_class_details; //show classification information
	QString language;

};


#endif // REPORTSETTINGS_H
