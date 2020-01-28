#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H
#include "cppNGSD_global.h"
#include "VariantType.h"
#include "Helper.h"
#include <QString>
#include <QDateTime>

struct CPPNGSDSHARED_EXPORT SomaticReportVariantConfiguration
{
	SomaticReportVariantConfiguration();
	bool showInReport() const;

	VariantType variant_type = VariantType::INVALID;
	int variant_index = -1;

	//exclusions
	bool exclude_artefact;
	bool exclude_low_tumor_content;
	bool exclude_low_copy_number;
	bool exclude_high_baf_deviation;
	bool exclude_other_reason;

	//Include (usually non-protein coding) variants
	QString include_variant_alteration;
	QString include_variant_description;

	QString comment;
};

class CPPNGSDSHARED_EXPORT SomaticReportConfiguration
{
public:
	SomaticReportConfiguration();

	QString createdBy() const;
	QDateTime createdAt() const;

	///Returns list containing all report variant configurations
	const QList<SomaticReportVariantConfiguration>& variantConfig() const;

	///Returns variant configuration for variant_index (index referes to index of main variant list!)
	const SomaticReportVariantConfiguration& variantConfig(int variant_index);

	QString targetFile() const;

	QList<int> variantIndices(VariantType type, bool only_selected, QString report_type = QString()) const;

	bool exists(VariantType type, int index) const;

	///sets / adds somatic variant configuration to list.
	bool set(const SomaticReportVariantConfiguration& config);

	const SomaticReportVariantConfiguration& get(VariantType type, int index) const;

	bool remove(VariantType type, int index);

	void setCreatedBy(QString user);
	void setCreatedAt(QDateTime time);
	void setTargetFile(QString target_bed);

	int count();

	void sortByPosition();

private:
	QList<SomaticReportVariantConfiguration> variant_config_;
	QString created_by_;
	QDateTime created_at_;
	QString target_file_;
};

#endif // SOMATICREPORTCONFIGURATION_H
