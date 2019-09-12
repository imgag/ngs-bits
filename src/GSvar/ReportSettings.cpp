#include "ReportSettings.h"

ReportVariantConfiguration::ReportVariantConfiguration()
	: variant_type(VariantType::SNVS_INDELS)
	, variant_index(-1)
	, type()
	, inheritance_mode()
	, de_novo(false)
	, mosaic(false)
	, comp_het(false)
	, comment()
{
}

bool ReportVariantConfiguration::showInReport() const
{
	return report == "add to report";
}

QIcon ReportVariantConfiguration::icon() const
{
	return QIcon(showInReport() ? QPixmap(":/Icons/Report_add.png") : QPixmap(":/Icons/Report exclude.png"));
}

QStringList ReportVariantConfiguration::getTypeOptions()
{
	//TODO take from NGSD once implemented (cache)
	return QStringList() << "diagnostic variant" << "candidate variant" << "incidental finding (ACMG)";
}

QStringList ReportVariantConfiguration::getReportOptions()
{
	//TODO take from NGSD once implemented (cache)
	return QStringList() << "add to report" <<  "no report: artefact" << "no report: allele frequency too high" << "no report: phenotype not matching" << "no report: pathomechanism not matching" << "no report: other reason";
}

QStringList ReportVariantConfiguration::getInheritanceModeOptions()
{
	//TODO take from NGSD once implemented (cache)
	return QStringList() << "n/a" << "AR" << "AD" << "AR+AD" << "XLR" << "XLD" << "XLR+XLD" << "MT";
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

bool ReportSettings::configurationExists(VariantType type, int index) const
{
	foreach(const ReportVariantConfiguration& var_conf, variant_config)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return true;
	}

	return false;
}

const ReportVariantConfiguration& ReportSettings::getConfiguration(VariantType type, int index) const
{
	foreach(const ReportVariantConfiguration& var_conf, variant_config)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return var_conf;
	}

	THROW(ArgumentException, "Report configuration not found for variant with index '" + QString::number(index) + "'!");
}

bool ReportSettings::setConfiguration(const ReportVariantConfiguration& config)
{
	for (int i=0; i<variant_config.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config[i];
		if (var_conf.variant_index==config.variant_index && var_conf.variant_type==config.variant_type)
		{
			variant_config[i] = config;
			return true;
		}
	}

	variant_config << config;
	return false;
}

bool ReportSettings::removeConfiguration(VariantType type, int index)
{
	for (int i=0; i<variant_config.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config[i];
		if (var_conf.variant_index==index && var_conf.variant_type==type)
		{
			variant_config.removeAt(i);
			return true;
		}
	}

	return false;
}

