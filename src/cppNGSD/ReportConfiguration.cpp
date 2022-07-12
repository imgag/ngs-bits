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

QStringList ReportVariantConfiguration::getTypeOptions()
{
	static QStringList types = NGSD().getEnum("report_configuration_variant", "type");
	return types;
}

QStringList ReportVariantConfiguration::getInheritanceModeOptions()
{
	static QStringList modes = NGSD().getEnum("report_configuration_variant", "inheritance");
	return modes;
}

QStringList ReportVariantConfiguration::getClassificationOptions()
{
	static QStringList modes = NGSD().getEnum("report_configuration_cnv", "class");
	return modes;
}


/*************************************** ReportConfiguration ***************************************/

ReportConfiguration::ReportConfiguration()
	: variant_config_()
	, created_by_(LoginManager::userLogin())
	, created_at_(QDateTime::currentDateTime())
	, last_updated_by_()
	, last_updated_at_(QDateTime())
	, finalized_by_()
	, finalized_at_(QDateTime())
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
		if (!report_type.isNull() && report_type!="all" && var_conf.report_type!=report_type) continue;

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

OtherCausalVariant ReportConfiguration::otherCausalVariant()
{
	return other_causal_variant_;
}

void ReportConfiguration::setOtherCausalVariant(const OtherCausalVariant& causal_variant)
{
	other_causal_variant_ = causal_variant;
	emit variantsChanged();
}

void ReportConfiguration::set(const ReportVariantConfiguration& config)
{
	bool updated_existing = false;
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==config.variant_index && var_conf.variant_type==config.variant_type)
		{
			variant_config_[i] = config;
			updated_existing = true;
			break;
		}
	}

	if (!updated_existing)
	{
		variant_config_ << config;
		sortByPosition();
	}

	emit variantsChanged();
}

void ReportConfiguration::remove(VariantType type, int index)
{
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==index && var_conf.variant_type==type)
		{
			variant_config_.removeAt(i);
			break;
		}
	}

	emit variantsChanged();
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

QString ReportConfiguration::lastUpdatedBy() const
{
	return last_updated_by_;
}

QDateTime ReportConfiguration::lastUpdatedAt() const
{
	return last_updated_at_;
}

QString ReportConfiguration::finalizedBy() const
{
	return finalized_by_;
}

QDateTime ReportConfiguration::finalizedAt() const
{
	return finalized_at_;
}

bool ReportConfiguration::isFinalized() const
{
	return !finalized_by_.isEmpty();
}

QString ReportConfiguration::history() const
{
	QStringList output;

	output << "The report configuration was created by " + created_by_ + " on " + created_at_.toString("dd.MM.yyyy") + " at " + created_at_.toString("hh:mm:ss") + ".";
	if (last_updated_by_!="") output << "It was last updated by " + last_updated_by_ + " on " +  last_updated_at_.toString("dd.MM.yyyy") + " at " + last_updated_at_.toString("hh:mm:ss") + ".";
	if (finalized_by_!="") output << "It was finalized by " + finalized_by_ + " on " +  finalized_at_.toString("dd.MM.yyyy") + " at " + finalized_at_.toString("hh:mm:ss") + ".";

	return output.join("\n");
}

QString ReportConfiguration::variantSummary() const
{
	//count by type and causal
	int c_small = 0;
	int c_small_causal = 0;
	int c_cnv = 0;
	int c_cnv_causal = 0;
	int c_sv = 0;
	int c_sv_causal = 0;
	foreach(const ReportVariantConfiguration& entry, variant_config_)
	{
		if (entry.variant_type==VariantType::SNVS_INDELS)
		{
			++c_small;
			if (entry.causal) ++c_small_causal;
		}
		else if (entry.variant_type==VariantType::CNVS)
		{
			++c_cnv;
			if (entry.causal) ++c_cnv_causal;
		}
		else if (entry.variant_type==VariantType::SVS)
		{
			++c_sv;
			if (entry.causal) ++c_sv_causal;
		}
	}

	QStringList output;
	output << ("small variants: " + QString::number(c_small));
	if (c_small_causal>0) output.last().append(" (" + QString::number(c_small_causal) + " causal)");
	output << ("CNVs: " + QString::number(c_cnv));
	if (c_cnv_causal>0) output.last().append(" (" + QString::number(c_cnv_causal) + " causal)");
	output << ("SVs: " + QString::number(c_sv));
	if (c_sv_causal>0) output.last().append(" (" + QString::number(c_sv_causal) + " causal)");
	OtherCausalVariant test = other_causal_variant_;
	if (other_causal_variant_.isValid())
	{
		output << "other causal variant: 1";
	}
	else
	{
		output << "other causal variant: 0";
	}

	return output.join("\n");
}

void ReportConfiguration::sortByPosition()
{
	std::sort(variant_config_.begin(), variant_config_.end(), [](const ReportVariantConfiguration& a, const ReportVariantConfiguration& b){return a.variant_index < b.variant_index;});
}

