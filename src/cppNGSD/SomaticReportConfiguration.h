#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H
#include "cppNGSD_global.h"
#include "VariantType.h"
#include <QString>


struct CPPNGSDSHARED_EXPORT SomaticReportVariantConfiguration
{
	SomaticReportVariantConfiguration();

	VariantType variant_type;
	int variant_index;
	QString report_type;

	//exclusions
	bool exclude_artefact;
	bool exlude_low_tumor_content;
	bool exclude_low_copy_number;
	bool exclude_high_baf_deviation;
	bool exclude_other_reason;

	//Include (usually non-protein coding) variants
	QString include_variant_alteration;
	QString include_variant_description;

	QString comment;

};

class SomaticReportConfiguration
{
public:
	SomaticReportConfiguration();
};

#endif // SOMATICREPORTCONFIGURATION_H
