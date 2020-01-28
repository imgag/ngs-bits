#include "SomaticReportSettings.h"
#include "SomaticReportHelper.h"


SomaticReportSettings::SomaticReportSettings()
	: report_config()
	, tumor_ps()
	, normal_ps()
	, filters()
	, report_type()
	, include_tum_content_clonality(false)
	, include_tum_content_snp_af(false)
	, include_tum_content_histological(false)
	, include_cov_statistics(false)
{
}

