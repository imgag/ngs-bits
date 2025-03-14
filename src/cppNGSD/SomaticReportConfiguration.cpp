#include <QFileInfo>
#include "SomaticReportConfiguration.h"
#include "NGSD.h"
#include "Settings.h"

SomaticReportVariantConfiguration::SomaticReportVariantConfiguration()
	: variant_type(VariantType::SNVS_INDELS)
	, variant_index(-1)
	, exclude_artefact(false)
	, exclude_other_reason(false)
	, exclude_low_tumor_content(false)
	, exclude_low_copy_number(false)
	, exclude_high_baf_deviation(false)
	, exclude_unclear_effect(false)
	, comment()
	, description()
	, include_variant_alteration()
	, include_variant_description()
	, manual_sv_start()
	, manual_sv_end()
	, manual_sv_hgvs_type()
	, manual_sv_hgvs_suffix()
	, manual_sv_start_bnd()
	, manual_sv_end_bnd()
	, manual_sv_hgvs_type_bnd()
	, manual_sv_hgvs_suffix_bnd()
	, rna_info()
{
}

SomaticReportGermlineVariantConfiguration::SomaticReportGermlineVariantConfiguration()
	: variant_index(-1)
	, tum_freq(std::numeric_limits<double>::quiet_NaN())
{
}

bool SomaticReportVariantConfiguration::showInReport() const
{
	return !(exclude_artefact || exclude_low_tumor_content || exclude_low_copy_number || exclude_high_baf_deviation || exclude_other_reason || exclude_unclear_effect);
}


bool SomaticReportVariantConfiguration::StringValidInt(QString string) const
{
	if (string.isEmpty()) return false;

	bool ok = false;
	int value = string.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

bool SomaticReportVariantConfiguration::manualSvStartValid() const
{
	return StringValidInt(manual_sv_start);
}

bool SomaticReportVariantConfiguration::manualSvEndValid() const
{
	return StringValidInt(manual_sv_end);
}

bool SomaticReportVariantConfiguration::manualSvStartBndValid() const
{
	return StringValidInt(manual_sv_start_bnd);
}

bool SomaticReportVariantConfiguration::manualSvEndBndValid() const
{
	return StringValidInt(manual_sv_end_bnd);
}






SomaticReportConfiguration::SomaticReportConfiguration()
	: variant_config_()
	, germ_variant_config_()
	, created_by_(Helper::userName())
	, created_at_(QDateTime::currentDateTime())
	, target_region_name_()
	, include_tum_content_clonality_(false)
	, include_tum_content_snp_af_(false)
	, include_tum_content_histological_(false)
	, include_msi_status_(false)
	, include_cnv_burden_(false)
	, include_mutation_burden_(true)
	, ploidy_(0)
	, hrd_statement_()
	, cnv_loh_count_(0)
	, cnv_tai_count_(0)
	, cnv_lst_count_(0)
	, cin_chromosomes_()
	, limitations_()
	, fusions_detected_(false)
	, tmb_reference_text_()
	, quality_()
	, filter_name_()
{
}

const QList<SomaticReportVariantConfiguration>& SomaticReportConfiguration::variantConfig() const
{
	return variant_config_;
}

const SomaticReportVariantConfiguration& SomaticReportConfiguration::variantConfig(int variant_index, VariantType type) const
{
	for(const auto& conf : variant_config_)
	{
		if(conf.variant_index == variant_index && conf.variant_type == type) return conf;
	}

	THROW(ArgumentException, "Could not find somatic variant configuration for index " + QString::number(variant_index));
}

const QList<SomaticReportGermlineVariantConfiguration>& SomaticReportConfiguration::variantConfigGermline() const
{
	return germ_variant_config_;
}

const SomaticReportGermlineVariantConfiguration& SomaticReportConfiguration::variantConfigGermline(int variant_index) const
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

void SomaticReportConfiguration::clearSomaticVariantConfigurations()
{
	variant_config_.clear();
}

void SomaticReportConfiguration::addSomaticVariantConfiguration(const SomaticReportVariantConfiguration &config)
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
			return;
		}
	}
	//set variant config (if not yet contained)
	variant_config_ << config;
	sortByPosition();
	return;
}

void SomaticReportConfiguration::clearGermlineVariantConfigurations()
{
	germ_variant_config_.clear();
}

void SomaticReportConfiguration::addGermlineVariantConfiguration(const SomaticReportGermlineVariantConfiguration& config)
{
	for(int i=0; i< germ_variant_config_.count(); ++i)
	{
		if(config.variant_index == germ_variant_config_[i].variant_index)
		{
			germ_variant_config_[i] = config;
			return;
		}
	}

	germ_variant_config_ << config;
	return;
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
QString SomaticReportConfiguration::targetRegionName() const
{
	return target_region_name_;
}
void SomaticReportConfiguration::setCreatedAt(QDateTime time)
{
	created_at_ = time;
}
void SomaticReportConfiguration::setCreatedBy(QString user)
{
	created_by_ = user;
}
void SomaticReportConfiguration::setTargetRegionName(QString target_name)
{
	target_region_name_ = target_name;
}
void SomaticReportConfiguration::sortByPosition()
{
	std::sort(variant_config_.begin(), variant_config_.end(), [](const SomaticReportVariantConfiguration& a, const SomaticReportVariantConfiguration& b){return a.variant_index < b.variant_index;});
}

bool SomaticReportConfiguration::includeTumContentByClonality() const
{
	return include_tum_content_clonality_;
}

void SomaticReportConfiguration::setIncludeTumContentByClonality(bool include_tum_content_clonality)
{
	include_tum_content_clonality_ = include_tum_content_clonality;
}

bool SomaticReportConfiguration::includeTumContentByMaxSNV() const
{
	return include_tum_content_snp_af_;
}

void SomaticReportConfiguration::setIncludeTumContentByMaxSNV(bool include_tum_content_snp_af)
{
	include_tum_content_snp_af_ = include_tum_content_snp_af;
}

bool SomaticReportConfiguration::includeTumContentByHistological() const
{
	return include_tum_content_histological_;
}

void SomaticReportConfiguration::setIncludeTumContentByHistological(bool include_tum_content_histological)
{
	include_tum_content_histological_ = include_tum_content_histological;
}

bool SomaticReportConfiguration::includeTumContentByEstimated() const
{
	return include_tum_content_estimated_;
}

void SomaticReportConfiguration::setIncludeTumContentByEstimated(bool include_tum_content_estimated)
{
	include_tum_content_estimated_ = include_tum_content_estimated;
}

double SomaticReportConfiguration::tumContentByEstimated() const
{
	return tum_content_estimated_;
}

void SomaticReportConfiguration::setTumContentByEstimated(double tum_content_estimated)
{
	tum_content_estimated_ = tum_content_estimated;
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

bool SomaticReportConfiguration::includeMutationBurden() const
{
	return include_mutation_burden_;
}

void SomaticReportConfiguration::setIncludeMutationBurden(bool include_mutation_burden)
{
	include_mutation_burden_ = include_mutation_burden;
}

const QList<QString>& SomaticReportConfiguration::cinChromosomes() const
{
	return cin_chromosomes_;
}

void SomaticReportConfiguration::setCinChromosomes(const QList<QString>& chromosomes)
{

	const QList<QString> values = {"chr1","chr2","chr3","chr4","chr5","chr6","chr7","chr8","chr9","chr10","chr11","chr12","chr13","chr14","chr15","chr16","chr17","chr18","chr19","chr20","chr21","chr22","chrX","chrY", "chrMT"};

	for(const auto& chr : chromosomes)
	{
		if(!values.contains(chr))
		{
			THROW(ArgumentException, "Chromosome " + chr +" does not match nomenclature in SomaticReportConfiguration::setCinChromosomes");
		}
	}


	cin_chromosomes_ = chromosomes;
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

QStringList SomaticReportConfiguration::quality() const
{
	return quality_;
}

void SomaticReportConfiguration::setQuality(QStringList qual)
{
	quality_ = qual;
}

QString SomaticReportConfiguration::limitations() const
{
	return limitations_;
}

void SomaticReportConfiguration::setLimitations(QString limitations)
{
	limitations_ = limitations;
}

QString SomaticReportConfiguration::filterName() const
{
	return filter_name_;
}

void SomaticReportConfiguration::setFilterName(QString filter)
{
	filter_name_ = filter;
}

FilterCascade SomaticReportConfiguration::filters() const
{
	return filters_;
}

void SomaticReportConfiguration::setFilters(FilterCascade filters)
{
	filters_ = filters;
}

int SomaticReportConfiguration::cnvLohCount() const
{
    return cnv_loh_count_;
}

void SomaticReportConfiguration::setCnvLohCount(int cnv_loh_count)
{
    cnv_loh_count_ = cnv_loh_count;
}

int SomaticReportConfiguration::cnvTaiCount() const
{
    return cnv_tai_count_;
}

void SomaticReportConfiguration::setCnvTaiCount(int cnv_tai_count)
{
    cnv_tai_count_ = cnv_tai_count;
}

int SomaticReportConfiguration::cnvLstCount() const
{
    return cnv_lst_count_;
}

void SomaticReportConfiguration::setCnvLstCount(int cnv_lst_count)
{
    cnv_lst_count_ = cnv_lst_count;
}

QString SomaticReportConfiguration::hrdStatement() const
{
	return hrd_statement_;
}

void SomaticReportConfiguration::setHrdStatement(const QString& hrd_statement)
{
	hrd_statement_ = hrd_statement;
}

double SomaticReportConfiguration::ploidy() const
{
	return ploidy_;
}

void SomaticReportConfiguration::setPloidy(double ploidy)
{
	ploidy_ = ploidy;
}

QDate SomaticReportConfiguration::evaluationDate() const
{
	return evaluation_date_;
}

void SomaticReportConfiguration::setEvaluationDate(QDate date)
{
	evaluation_date_ = date;
}


