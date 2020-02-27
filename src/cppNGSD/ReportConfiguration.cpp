#include "ReportConfiguration.h"
#include "Exceptions.h"
#include "NGSD.h"
#include "Helper.h"
#include "LoginManager.h"

/*************************************** ReportVariantConfiguration ***************************************/

ReportVariantConfiguration::ReportVariantConfiguration()
	: variant_type(VariantType::SNVS_INDELS)
	, variant_index(-1)
	, report_type()
	, causal(false)
	, classification("n/a")
	, inheritance("n/a")
	, de_novo(false)
	, mosaic(false)
	, comp_het(false)
	, exclude_artefact(false)
	, exclude_frequency(false)
	, exclude_phenotype(false)
	, exclude_mechanism(false)
	, exclude_other(false)
	, comments()
	, comments2()
{
}

bool ReportVariantConfiguration::showInReport() const
{
	return !(exclude_artefact || exclude_frequency || exclude_phenotype || exclude_mechanism || exclude_other);
}

QStringList ReportVariantConfiguration::getTypeOptions(bool test_db)
{
	static QStringList types = NGSD(test_db).getEnum("report_configuration_variant", "type");
	return types;
}

QStringList ReportVariantConfiguration::getInheritanceModeOptions(bool test_db)
{
	static QStringList modes = NGSD(test_db).getEnum("report_configuration_variant", "inheritance");
	return modes;
}

QStringList ReportVariantConfiguration::getClassificationOptions(bool test_db)
{
	static QStringList modes = NGSD(test_db).getEnum("report_configuration_cnv", "class");
	return modes;
}


/*************************************** ReportConfiguration ***************************************/

ReportConfiguration::ReportConfiguration()
	: variant_config_()
	, created_by_(LoginManager::user())
	, created_at_(QDateTime::currentDateTime())
{
}

const QList<ReportVariantConfiguration>& ReportConfiguration::variantConfig() const
{
	return variant_config_;
}

QList<int> ReportConfiguration::variantIndices(VariantType type, bool only_selected, QString report_type) const
{
	QList<int> output;

	foreach(const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_type!=type) continue;
		if (only_selected && !var_conf.showInReport()) continue;
		if (!report_type.isNull() && var_conf.report_type!=report_type) continue;

		output << var_conf.variant_index;
	}

	std::sort(output.begin(), output.end());

	return output;
}

bool ReportConfiguration::exists(VariantType type, int index) const
{
	foreach(const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return true;
	}

	return false;
}

const ReportVariantConfiguration& ReportConfiguration::get(VariantType type, int index) const
{
	foreach(const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return var_conf;
	}

	THROW(ArgumentException, "Report configuration not found for variant with index '" + QString::number(index) + "'!");
}

bool ReportConfiguration::set(const ReportVariantConfiguration& config)
{
	//set variant config
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==config.variant_index && var_conf.variant_type==config.variant_type)
		{
			variant_config_[i] = config;
			return true;
		}
	}
	variant_config_ << config;
	sortByPosition();

	return false;
}

bool ReportConfiguration::remove(VariantType type, int index)
{
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==index && var_conf.variant_type==type)
		{
			variant_config_.removeAt(i);
			return true;
		}
	}

	return false;
}

QString ReportConfiguration::createdBy() const
{
	return created_by_;
}

void ReportConfiguration::setCreatedBy(QString user_name)
{
	created_by_ = user_name;
}

QDateTime ReportConfiguration::createdAt() const
{
	return created_at_;
}

void ReportConfiguration::setCreatedAt(QDateTime time)
{
	created_at_ = time;
}

void ReportConfiguration::sortByPosition()
{
	std::sort(variant_config_.begin(), variant_config_.end(), [](const ReportVariantConfiguration& a, const ReportVariantConfiguration& b){return a.variant_index < b.variant_index;});
}

