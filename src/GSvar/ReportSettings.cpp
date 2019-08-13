#include "ReportSettings.h"

ReportVariantConfiguration::ReportVariantConfiguration()
	: variant_type(VariantType::SNVS_INDELS)
	, variant_index(-1)
	, type("")
	, de_novo(false)
	, inheritance_mode("")
{
}

bool ReportVariantConfiguration::showInReport() const
{
	return type.startsWith("report:");
}

QStringList ReportVariantConfiguration::getTypeOptions()
{
	//TODO take from NGSD once implemented (cache)
	return QStringList() << "report: causal" << "report: candidategene" << "report: scientific" << "report: incidental finding (ACMG)" << "no report: artefact" << "no report: no diseaseassociation";
}

QStringList ReportVariantConfiguration::getInheritanceModeOptions()
{
	//TODO take from NGSD once implemented (cache)
	return QStringList() << "autosomal recessive" << "autosomal dominant" << "autosomal recessive/dominant" << "x-linked recessive" << "x-linked dominant" << "x-linked recessive/dominant" << "mitochondrial" << "somatic";
}

ReportSettings::ReportSettings()
	: diag_status()
	, variant_config()
	, show_coverage_details(true)
	, min_depth(20)
	, roi_low_cov(false)
	, recalculate_avg_depth(false)
	, show_tool_details(false)
	, show_omim_table(true)
	, show_class_details(false)
	, language("german")
{
}

QList<int> ReportSettings::variantIndices(VariantType type, bool only_selected) const
{
	QList<int> output;

	foreach(const ReportVariantConfiguration& var_conf, variant_config)
	{
		if (var_conf.variant_type!=type) continue;
		if (only_selected && !var_conf.showInReport()) continue;

		output << var_conf.variant_index;
	}

	std::sort(output.begin(), output.end());

	return output;
}

