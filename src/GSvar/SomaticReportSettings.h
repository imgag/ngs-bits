#ifndef SOMATICREPORTSETTINGS_H
#define SOMATICREPORTSETTINGS_H

#include "NGSD.h"
#include "VariantType.h"
#include <QStringList>


struct SomaticReportSettings
{
	SomaticReportSettings();

	SomaticReportConfiguration report_config;
	QString region_of_interest; //region from which variants will be shown in report
};

#endif // SOMATICREPORTSETTINGS_H
