#ifndef SOMATICREPORTSETTINGS_H
#define SOMATICREPORTSETTINGS_H

#include "SomaticReportConfiguration.h"
#include "FilterCascade.h"

struct SomaticReportSettings
{
	SomaticReportSettings();

	SomaticReportConfiguration report_config;
	FilterCascade filters;

	QString report_type;
	QString target_bed_file;

};

#endif // SOMATICREPORTSETTINGS_H
