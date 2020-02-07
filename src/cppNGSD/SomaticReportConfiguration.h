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

	///Returns list containing all report variant configurations
	const QList<SomaticReportVariantConfiguration>& variantConfig() const;

	///Returns variant configuration for variant_index (index referes to index of main variant list!)
	const SomaticReportVariantConfiguration& variantConfig(int variant_index) const;

	QList<int> variantIndices(VariantType type, bool only_selected, QString report_type = QString()) const;

	bool exists(VariantType type, int index) const;

	///sets / adds somatic variant configuration to list.
	bool set(const SomaticReportVariantConfiguration& config);

	const SomaticReportVariantConfiguration& get(VariantType type, int index) const;

	bool remove(VariantType type, int index);

	int count();

	void sortByPosition();



	QString targetFile() const;
	void setTargetFile(QString target_bed);

	QString createdBy() const;
	void setCreatedBy(QString user);

	QDateTime createdAt() const;
	void setCreatedAt(QDateTime time);

	bool tumContentByClonality() const;
	void setTumContentByClonality(bool include_tum_content_clonality);

	bool tumContentByMaxSNV() const;
	void setTumContentByMaxSNV(bool include_tum_content_snp_af);

	bool tumContentByHistological() const;
	void setTumContentByHistological(bool include_tum_content_histological);

	bool msiStatus() const;
	void setMsiStatus(bool include_msi_status);

	bool cnvBurden() const;
	void setCnvBurden(bool include_cnv_burden);

	int hrdScore() const;
	void setHrdScore(int score);

	bool cinHint() const;
	void setCinHint(bool include_cin_hint);

	QString tmbReferenceText() const;
	void setTmbReferenceText(QString ref_text);

	QString quality() const;
	void setQuality(QString qual);

private:
	QList<SomaticReportVariantConfiguration> variant_config_;
	QString created_by_;
	QDateTime created_at_;
	QString target_file_;

	bool include_tum_content_clonality_;
	bool include_tum_content_snp_af_;
	bool include_tum_content_histological_;

	bool include_msi_status_;
	bool include_cnv_burden_;
	int hrd_score_; //0 no HRD; 1,2 low; 3 intermediate HRD; 4,5 high HRD
	bool include_cin_hint_;

	//Reference text for tumor mutation burden
	QString tmb_reference_text_;

	//Quality description
	QString quality_;
};

#endif // SOMATICREPORTCONFIGURATION_H
