#include "SomaticReportSettings.h"


SomaticReportSettings::SomaticReportSettings()
	: report_config()
	, tumor_ps()
	, normal_ps()
	, filters()
{
}

VariantList SomaticReportSettings::filterVariants(const VariantList &snvs, const SomaticReportSettings& sett)
{
	QSet<int> variant_indices = sett.report_config.variantIndices(VariantType::SNVS_INDELS, false).toSet();

	VariantList result;

	result.copyMetaData(snvs);

	FilterResult filter_res = sett.filters.apply(snvs); //does not regard "include" result of report_config

	//filter for target region
	if(sett.target_region_filter.regions.count() > 0)
	{
		FilterRegions::apply(snvs, sett.target_region_filter.regions, filter_res);
	}


	//Adapt filter results to results from report settings
	for(int index : variant_indices)
	{
		filter_res.flags()[index] = sett.report_config.variantConfig(index, VariantType::SNVS_INDELS).showInReport();
	}


	result.addAnnotation("alt_var_alteration","If an alternative text for protein change is specified in report config, this is stored here.", "");
	result.addAnnotation("alt_var_description", "Alternate description text for variant alteration", "");
	for(int i=0; i<snvs.count(); ++i)
	{
		if(!filter_res.flags()[i]) continue;

		result.append(snvs[i]);

		//add additional report config info into new empty annotation columns
		if(variant_indices.contains(i) && sett.report_config.variantConfig(i, VariantType::SNVS_INDELS).showInReport())
		{
			result[result.count()-1].annotations().append(sett.report_config.variantConfig(i, VariantType::SNVS_INDELS).include_variant_alteration.toUtf8());
			result[result.count()-1].annotations().append(sett.report_config.variantConfig(i, VariantType::SNVS_INDELS).include_variant_description.toUtf8());
		}
		else
		{
			result[result.count()-1].annotations().append({"",""}); //empty annotation columns
		}
	}

	return result;
}

VariantList SomaticReportSettings::filterGermlineVariants(const VariantList &germl_snvs, const SomaticReportSettings &sett)
{
	QSet<int> variant_indices = sett.report_config.variantIndicesGermline().toSet();

	VariantList result;

	result.copyMetaData(germl_snvs);

	result.addAnnotation("freq_in_tum", "Frequency of variant which was found in normal tissue within the tumor sample.");
	result.addAnnotation("depth_in_tum", "Depth of variant which was found in normal tissue within the tumor sample.");

	for(int i=0; i< germl_snvs.count(); ++i)
	{
		if(variant_indices.contains(i))
		{
			result.append(germl_snvs[i]);
			result[result.count()-1].annotations().append( QByteArray::number(sett.report_config.variantConfigGermline(i).tum_freq) );
			result[result.count()-1].annotations().append( QByteArray::number(sett.report_config.variantConfigGermline(i).tum_depth) );
		}
	}

	return result;


}

CnvList SomaticReportSettings::filterCnvs(const CnvList &cnvs, const SomaticReportSettings &sett)
{
	QSet<int> cnv_indices = sett.report_config.variantIndices(VariantType::CNVS, false).toSet();

	CnvList result;
	result.copyMetaData(cnvs);

	QBitArray cnv_flags(cnvs.count(), true);

	for(int index : cnv_indices)
	{
		cnv_flags[index] = sett.report_config.variantConfig(index, VariantType::CNVS).showInReport();
	}

	for(int i=0; i<cnvs.count(); ++i)
	{
		if(!cnv_flags[i]) continue;

		result.append(cnvs[i]);
	}
	return result;
}
