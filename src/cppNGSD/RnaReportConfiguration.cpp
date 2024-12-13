#include <QFileInfo>
#include "RnaReportConfiguration.h"
#include "NGSD.h"
#include "Settings.h"

RnaReportFusionConfiguration::RnaReportFusionConfiguration()
	: variant_index(-1)
	, exclude_artefact(false)
	, exclude_low_tumor_content(false)
	, exclude_low_evidence(false)
	, exclude_other_reason(false)
	, comment()
{
}

bool RnaReportFusionConfiguration::showInReport() const
{
	return !(exclude_artefact || exclude_low_tumor_content || exclude_low_evidence || exclude_other_reason);
}


RnaReportConfiguration::RnaReportConfiguration()
	: fusion_config_()
	, created_by_(Helper::userName())
	, created_at_(QDateTime::currentDateTime())
{
}

const QList<RnaReportFusionConfiguration>& RnaReportConfiguration::fusionConfig() const
{
	return fusion_config_;
}

const RnaReportFusionConfiguration& RnaReportConfiguration::fusionConfig(int variant_index) const
{
	for(const auto& conf : fusion_config_)
	{
		if(conf.variant_index == variant_index) return conf;
	}

	THROW(ArgumentException, "Could not find somatic variant configuration for index " + QString::number(variant_index));
}

QList<int> RnaReportConfiguration::fusionIndices(bool only_selected) const
{
	QList<int> output;

	for(const auto& var_conf : fusion_config_)
	{
		if (only_selected && !var_conf.showInReport()) continue;
		output << var_conf.variant_index;
	}

	std::sort(output.begin(), output.end());

	return output;
}

bool RnaReportConfiguration::exists(int index) const
{
	for(const auto& var_conf : fusion_config_)
	{
		if (var_conf.variant_index==index) return true;
	}

	return false;
}

void RnaReportConfiguration::clearRnaFusionConfigurations()
{
	fusion_config_.clear();
}

void RnaReportConfiguration::addRnaFusionConfiguration(const RnaReportFusionConfiguration &config)
{	
	//set variant config (if already contained in list)
	for (int i=0; i<fusion_config_.count(); ++i)
	{
		const RnaReportFusionConfiguration& var_conf = fusion_config_[i];
		if (var_conf.variant_index==config.variant_index)
		{
			fusion_config_[i] = config;
			return;
		}
	}
	//set variant config (if not yet contained)
	fusion_config_ << config;
	sortByIndex();
	return;
}

const RnaReportFusionConfiguration& RnaReportConfiguration::get(int index) const
{
	for(const auto& var_conf : fusion_config_)
	{
		if (var_conf.variant_index==index) return var_conf;
	}
	THROW(ArgumentException, "Report configuration not found for fusion with index '" + QString::number(index) + "'!");
}

bool RnaReportConfiguration::remove(int index)
{
	for(int i=0; i<fusion_config_.count(); ++i)
	{
		const auto& var_conf = fusion_config_[i];
		if (var_conf.variant_index==index)
		{
			fusion_config_.removeAt(i);
			return true;
		}
	}
	return false;
}

int RnaReportConfiguration::count() const
{
	return fusion_config_.count();
}

QDateTime RnaReportConfiguration::createdAt() const
{
	return created_at_;
}
void RnaReportConfiguration::setCreatedAt(QDateTime time)
{
	created_at_ = time;
}
QString RnaReportConfiguration::createdBy() const
{
	return created_by_;
}
void RnaReportConfiguration::setCreatedBy(QString user)
{
	created_by_ = user;
}

void RnaReportConfiguration::sortByIndex()
{
	std::sort(fusion_config_.begin(), fusion_config_.end(), [](const RnaReportFusionConfiguration& a, const RnaReportFusionConfiguration& b){return a.variant_index < b.variant_index;});
}

