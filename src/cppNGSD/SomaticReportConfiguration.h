#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H
#include "cppNGSD_global.h"
#include "VariantType.h"
#include "Helper.h"
#include "ReportConfiguration.h"
#include <Chromosome.h>
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

struct CPPNGSDSHARED_EXPORT SomaticReportGermlineVariantConfiguration
{
	SomaticReportGermlineVariantConfiguration();

	int variant_index = -1;
	double tum_freq = std::numeric_limits<double>::quiet_NaN(); //allele frequency of variant in tumor sample
	double tum_depth = std::numeric_limits<double>::quiet_NaN(); //depth of variant in tumor sample
};

class CPPNGSDSHARED_EXPORT SomaticReportConfiguration
{
public:
	SomaticReportConfiguration();

	///Returns list containing all report variant configurations
	const QList<SomaticReportVariantConfiguration>& variantConfig() const;
	///Returns variant configuration for variant_index (index referes to index of main variant list!)
	const SomaticReportVariantConfiguration& variantConfig(int variant_index, VariantType type) const;
	///Returns all variant configurations for all (germline!) report variant configurations
	const QList<SomaticReportGermlineVariantConfiguration>& variantConfigGermline() const;
	///Returns variant configuration for (germline!) variant_index (index referes to index of germline variant list!)
	const SomaticReportGermlineVariantConfiguration& variantConfigGermline(int variant_index) const;

	///all variant indices in configuration
	QList<int> variantIndices(VariantType type, bool only_selected) const;
	QList<int> variantIndicesGermline() const;

	///Check whether variant configuration exists for index
	bool exists(VariantType type, int index) const;

	///sets / adds somatic variant configuration to list.
	bool set(const SomaticReportVariantConfiguration& config);
	///sets /adds germline variant to somatic report configuration
	bool setGermline(const SomaticReportGermlineVariantConfiguration& config);

	///Somatic variant report configuration according type and index in file
	const SomaticReportVariantConfiguration& get(VariantType type, int index) const;
	///Somatic germline (!) variant report configuration according type and index in germline file
	const SomaticReportGermlineVariantConfiguration& getGermline(int index) const;

	bool remove(VariantType type, int index);

	bool removeGermline(int index);

	///returns variant count of somatic variants
	int count() const;
	///returns variant count of related germline variants
	int countGermline() const;

	void sortByPosition();

	///GETTER/SETTER

	QString targetRegionName() const;
	void setTargetRegionName(QString target_name);

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

	const QList<QString>& cinChromosomes() const;
	///Setter for CIN chromosomes, pass argument in the form {"chr1", "chr2", ...}
	void setCinChromosomes(const QList<QString>& chromosomes);

	bool fusionsDetected() const;
	void setFusionsDetected(bool detected);

	QString tmbReferenceText() const;
	void setTmbReferenceText(QString ref_text);

	QString quality() const;
	void setQuality(QString qual);

	QString limitations() const;
	void setLimitations(QString limitations);

	QString filter() const;
	void setFilter(QString filter);

private:
	QList<SomaticReportVariantConfiguration> variant_config_;

	QList<SomaticReportGermlineVariantConfiguration> germ_variant_config_;

	QString created_by_;
	QDateTime created_at_;
	QString target_region_name_;

	bool include_tum_content_clonality_;
	bool include_tum_content_snp_af_;
	bool include_tum_content_histological_;

	bool include_msi_status_;
	bool include_cnv_burden_;
	int hrd_score_; //0 no HRD; 1,2 low; 3 intermediate HRD; 4,5 high HRD

	//list of instable chromosomes in the form {"chr1", "chr2" ....}
	QList<QString> cin_chromosomes_;

	QString limitations_;

	bool fusions_detected_;

	//Reference text for tumor mutation burden
	QString tmb_reference_text_;

	//Quality description
	QString quality_;

	//Name of filter
	QString filter_;
};

#endif // SOMATICREPORTCONFIGURATION_H
