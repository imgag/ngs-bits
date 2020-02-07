#ifndef SOMATICREPORTSETTINGS_H
#define SOMATICREPORTSETTINGS_H

#include "SomaticReportConfiguration.h"
#include "FilterCascade.h"
#include "NGSD.h"
#include "VariantList.h"

struct SomaticReportSettings
{
	///Default constructor
	SomaticReportSettings();

	SomaticReportConfiguration report_config;
	QString tumor_ps;
	QString normal_ps;

	FilterCascade filters;

	bool include_gap_statistics;

	QByteArray cancer_type;

	///returns variant list according filters and include/exclude report_config settings. Include from settings will overwrite FilterCascade entry
	static VariantList filterVariants(const VariantList& snvs, const SomaticReportSettings& sett);
	///returns cnv list according excluded cnvs from somatic report_config settings
	static CnvList filterCnvs(const CnvList& cnvs, const SomaticReportSettings& sett);
};

#endif // SOMATICREPORTSETTINGS_H
