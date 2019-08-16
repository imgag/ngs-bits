#ifndef REPORTSETTINGS_H
#define REPORTSETTINGS_H

#include "VariantType.h"
#include "NGSD.h"
#include <QStringList>
#include <QIcon>


///Variant meta data for report.
struct ReportVariantConfiguration
{
	///Default constructor
	ReportVariantConfiguration();
	///Returns if the variant is to be shown in the report
	bool showInReport() const;
	///Returns the icon for the variant (depending on type)
	QIcon icon() const;

	VariantType variant_type;
	int variant_index;

	QString type;
	QString inheritance_mode;
	bool de_novo;
	bool mosaic;
	bool comp_het;
	QString comment;
	//TODO add second-look (how? user name?)

	static QStringList getTypeOptions();
	static QStringList getInheritanceModeOptions();
};

///Report meta data.
class ReportSettings
{
public:
	///Default constructor
	ReportSettings();

	///Returns if a report configuration exists for the variant.
	bool configurationExists(VariantType type, int index) const;
	///Returns the matching report configuration (throws an error if not found).
	const ReportVariantConfiguration& getConfiguration(VariantType type, int index) const;
	///Sets the report configuration for the variant. Returns if it already existed.
	bool setConfiguration(const ReportVariantConfiguration& config);
	///Removes the mathcing configuration. Returns if a configuration was removed.
	bool removeConfiguration(VariantType type, int index);
	///Returns indices of the matching variants .
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
