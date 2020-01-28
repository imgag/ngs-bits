#ifndef SOMATICREPORTSETTINGS_H
#define SOMATICREPORTSETTINGS_H

#include "SomaticReportConfiguration.h"
#include "FilterCascade.h"
#include "NGSD.h"

struct SomaticReportSettings
{
	///Default constructor
	SomaticReportSettings();

	SomaticReportConfiguration report_config;
	QString tumor_ps;
	QString normal_ps;

	FilterCascade filters;

	QString report_type;

	bool include_tum_content_clonality;
	bool include_tum_content_snp_af;
	bool include_tum_content_histological;
	bool include_cov_statistics;
};

#endif // SOMATICREPORTSETTINGS_H
