#include <QFileInfo>
#include "SomaticReportConfiguration.h"
#include "NGSD.h"
#include "Settings.h"

SomaticReportVariantConfiguration::SomaticReportVariantConfiguration()
	: variant_type(VariantType::SNVS_INDELS)
	, variant_index(-1)
	, exclude_artefact(false)
	, exclude_low_tumor_content(false)
	, exclude_low_copy_number(false)
	, exclude_high_baf_deviation(false)
	, exclude_other_reason(false)
	, include_variant_alteration()
	, include_variant_description()
	, comment()
{
}

SomaticReportGermlineVariantConfiguration::SomaticReportGermlineVariantConfiguration()
	: variant_index(-1)
{
}

bool SomaticReportVariantConfiguration::showInReport() const
{
	return !(exclude_artefact || exclude_low_tumor_content || exclude_low_copy_number || exclude_high_baf_deviation || exclude_other_reason);
}

SomaticReportConfiguration::SomaticReportConfiguration()
	: variant_config_()
	, created_by_(Helper::userName())
	, created_at_(QDateTime::currentDateTime())
	, include_tum_content_clonality_(false)
	, include_tum_content_snp_af_(false)
	, include_tum_content_histological_(false)
	, include_msi_status_(false)
	, include_cnv_burden_(false)
	, hrd_score_(0)
	, include_cin_hint_(false)
	, fusions_detected_(false)
{
}

const QList<SomaticReportVariantConfiguration>& SomaticReportConfiguration::variantConfig() const
{
	return variant_config_;
}

const SomaticReportVariantConfiguration& SomaticReportConfiguration::variantConfig(int variant_index) const
{
	for(const auto& conf : variant_config_)
	{
		if(conf.variant_index == variant_index) return conf;
	}

	THROW(ArgumentException, "Could not find somatic variant configuration for index " + QString::number(variant_index));
}

const QList<SomaticReportGermlineVariantConfiguration>& SomaticReportConfiguration::variantConfigGermline() const
{
	return germ_variant_config_;
}

const SomaticReportGermlineVariantConfiguration SomaticReportConfiguration::variantConfigGermline(int variant_index) const
{
	for(const auto& conf : germ_variant_config_)
	{
		if(conf.variant_index == variant_index) return conf;
	}
	THROW(ArgumentException, "Could not find somatic variant configuration for germline index " + QString::number(variant_index));
}

QList<int> SomaticReportConfiguration::variantIndices(VariantType type, bool only_selected) const
{
	QList<int> output;

	for(const auto& var_conf : variant_config_)
	{
		if (var_conf.variant_type!=type) continue;
		if (only_selected && !var_conf.showInReport()) continue;
		output << var_conf.variant_index;
	}

	std::sort(output.begin(), output.end());

	return output;
}

QList<int> SomaticReportConfiguration::variantIndicesGermline() const
{
	QList<int> out;
	for(const auto& var_conf : germ_variant_config_)
	{
		out << var_conf.variant_index;
	}
	return out;
}


bool SomaticReportConfiguration::exists(VariantType type, int index) const
{
	for(const auto& var_conf : variant_config_)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return true;
	}

	return false;
}

bool SomaticReportConfiguration::set(const SomaticReportVariantConfiguration &config)
{	
	if(config.variant_type == VariantType::INVALID)
	{
		THROW(ArgumentException, "Cannot set somatic report configuration. VariantType for variant index " + QByteArray::number(config.variant_index) + " is invalid in SomaticReportConfiguration::set");
	}
	if(config.variant_type == VariantType::SNVS_INDELS && (!config.include_variant_alteration.isEmpty() || !config.include_variant_description.isEmpty()) && !config.showInReport())
	{
		THROW(ArgumentException, "Cannot set somatic report configuration. Variant Configuration for variant index " + QByteArray::number(config.variant_index) + " contains both include and exclude switches.");
	}

	//set variant config (if already contained in list)
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const SomaticReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==config.variant_index && var_conf.variant_type==config.variant_type)
		{
			variant_config_[i] = config;
			return true;
		}
	}
	//set variant config (if not yet contained)
	variant_config_ << config;

	sortByPosition();

	return false;
}

bool SomaticReportConfiguration::setGermline(const SomaticReportGermlineVariantConfiguration& config)
{
	for(int i=0; i< germ_variant_config_.count(); ++i)
	{
		if(config.variant_index == germ_variant_config_[i].variant_index)
		{
			germ_variant_config_[i] = config;
			return true;
		}
	}

	germ_variant_config_ << config;
	return false;
}

const SomaticReportVariantConfiguration& SomaticReportConfiguration::get(VariantType type, int index) const
{
	for(const auto& var_conf : variant_config_)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return var_conf;
	}
	THROW(ArgumentException, "Report configuration not found for variant with index '" + QString::number(index) + "'!");
}

const SomaticReportGermlineVariantConfiguration& SomaticReportConfiguration::getGermline(int index) const
{
	for(const auto& var_conf : germ_variant_config_)
	{
		if(var_conf.variant_index==index) return var_conf;
	}
	THROW(ArgumentException, "Somatic Report configuration not found for germline variant with index '" + QString::number(index) + "'!");
}

bool SomaticReportConfiguration::remove(VariantType type, int index)
{
	for(int i=0; i<variant_config_.count(); ++i)
	{
		const auto& var_conf = variant_config_[i];
		if (var_conf.variant_index==index && var_conf.variant_type==type)
		{
			variant_config_.removeAt(i);
			return true;
		}
	}
	return false;
}

bool SomaticReportConfiguration::removeGermline(int index)
{
	for(int i=0; i< germ_variant_config_.count(); ++i)
	{
		if(germ_variant_config_[i].variant_index == index)
		{
			germ_variant_config_.removeAt(i);
			return true;
		}
	}
	return false;
}

int SomaticReportConfiguration::count() const
{
	return variant_config_.count();
}

int SomaticReportConfiguration::countGermline() const
{
	return germ_variant_config_.count();
}

QDateTime SomaticReportConfiguration::createdAt() const
{
	return created_at_;
}
QString SomaticReportConfiguration::createdBy() const
{
	return created_by_;
}
QString SomaticReportConfiguration::targetFile() const
{
	return target_file_;
}
void SomaticReportConfiguration::setCreatedAt(QDateTime time)
{
	created_at_ = time;
}
void SomaticReportConfiguration::setCreatedBy(QString user)
{
	created_by_ = user;
}
void SomaticReportConfiguration::setTargetFile(QString target_bed)
{
	if(target_bed != "") target_file_ = QFileInfo(target_bed).fileName();
	else target_file_ = "";
}
void SomaticReportConfiguration::sortByPosition()
{
	std::sort(variant_config_.begin(), variant_config_.end(), [](const SomaticReportVariantConfiguration& a, const SomaticReportVariantConfiguration& b){return a.variant_index < b.variant_index;});
}

bool SomaticReportConfiguration::tumContentByClonality() const
{
	return include_tum_content_clonality_;
}

void SomaticReportConfiguration::setTumContentByClonality(bool include_tum_content_clonality)
{
	include_tum_content_clonality_ = include_tum_content_clonality;
}

bool SomaticReportConfiguration::tumContentByMaxSNV() const
{
	return include_tum_content_snp_af_;
}

void SomaticReportConfiguration::setTumContentByMaxSNV(bool include_tum_content_snp_af)
{
	include_tum_content_snp_af_ = include_tum_content_snp_af;
}

bool SomaticReportConfiguration::tumContentByHistological() const
{
	return include_tum_content_histological_;
}

void SomaticReportConfiguration::setTumContentByHistological(bool include_tum_content_histological)
{
	include_tum_content_histological_ = include_tum_content_histological;
}

bool SomaticReportConfiguration::msiStatus() const
{
	return include_msi_status_;
}

void SomaticReportConfiguration::setMsiStatus(bool include_msi_status)
{
	include_msi_status_ = include_msi_status;
}

bool SomaticReportConfiguration::cnvBurden() const
{
	return include_cnv_burden_;
}

void SomaticReportConfiguration::setCnvBurden(bool include_cnv_burden)
{
	include_cnv_burden_ = include_cnv_burden;
}

int SomaticReportConfiguration::hrdScore() const
{
	return hrd_score_;
}

void SomaticReportConfiguration::setHrdScore(int hrd_score)
{
	if(hrd_score <= 5)	hrd_score_ = hrd_score;
	else hrd_score_ = 0;
}

bool SomaticReportConfiguration::cinHint() const
{
	return include_cin_hint_;
}

void SomaticReportConfiguration::setCinHint(bool include_cin_hint)
{
	include_cin_hint_ = include_cin_hint;
}

bool SomaticReportConfiguration::fusionsDetected() const
{
	return fusions_detected_;
}

void SomaticReportConfiguration::setFusionsDetected(bool detected)
{
	fusions_detected_ = detected;
}

QString SomaticReportConfiguration::tmbReferenceText() const
{
	return tmb_reference_text_;
}

void SomaticReportConfiguration::setTmbReferenceText(QString ref_text)
{
	tmb_reference_text_ = ref_text.mid(0,200); //NGSD schema allows ref text up to 200 chars
}

QString SomaticReportConfiguration::quality() const
{
	return quality_;
}

void SomaticReportConfiguration::setQuality(QString qual)
{
	quality_ = qual;
}


