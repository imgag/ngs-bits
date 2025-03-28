#ifndef SOMATICREPORTCONFIGURATION_H
#define SOMATICREPORTCONFIGURATION_H
#include "cppNGSD_global.h"
#include "VariantType.h"
#include "Helper.h"
#include "ReportConfiguration.h"
#include "FilterCascade.h"
#include <Chromosome.h>
#include <QString>
#include <QDateTime>

struct CPPNGSDSHARED_EXPORT SomaticReportVariantConfiguration
{
	SomaticReportVariantConfiguration();
	bool showInReport() const;

	bool manualSvStartValid() const;
	bool manualSvEndValid() const;
	bool manualSvStartBndValid() const;
	bool manualSvEndBndValid() const;

	VariantType variant_type = VariantType::INVALID;
	int variant_index = -1;

	//exclusions
	bool exclude_artefact;
	bool exclude_other_reason;
	bool exclude_low_tumor_content;
	bool exclude_low_copy_number;
	bool exclude_high_baf_deviation;
	bool exclude_unclear_effect;

	QString comment;
	QString description;

	//Include (usually non-protein coding) variants
	QString include_variant_alteration;
	QString include_variant_description;

	//manual curation of SVs
	QString manual_sv_start;
	QString manual_sv_end;
	QString manual_sv_hgvs_type;
	QString manual_sv_hgvs_suffix;
	QString manual_sv_start_bnd;
	QString manual_sv_end_bnd;
	QString manual_sv_hgvs_type_bnd;
	QString manual_sv_hgvs_suffix_bnd;

	QString rna_info;

private:
	bool StringValidInt(QString string) const;
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

	/// clears the current list of somatic variant configurations.
	void clearSomaticVariantConfigurations();
	/// adds somatic variant configuration to list if variant is already contained it updates the config.
	void addSomaticVariantConfiguration(const SomaticReportVariantConfiguration& config);
	/// clears the current list of germline variant configurations.
	void clearGermlineVariantConfigurations();
	/// adds germline variant to somatic report configuration if variant is already contained it updates the config.
	void addGermlineVariantConfiguration(const SomaticReportGermlineVariantConfiguration& config);

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

	bool includeTumContentByClonality() const;
	void setIncludeTumContentByClonality(bool include_tum_content_clonality);

	bool includeTumContentByMaxSNV() const;
	void setIncludeTumContentByMaxSNV(bool include_tum_content_snp_af);

	bool includeTumContentByHistological() const;
	void setIncludeTumContentByHistological(bool include_tum_content_histological);

	bool includeTumContentByEstimated() const;
	void setIncludeTumContentByEstimated(bool include_tum_content_estimated);

	double tumContentByEstimated() const;
	void setTumContentByEstimated(double tum_content_estimated);

	bool msiStatus() const;
	void setMsiStatus(bool include_msi_status);

	bool cnvBurden() const;
	void setCnvBurden(bool include_cnv_burden);

	bool includeMutationBurden() const;
	void setIncludeMutationBurden(bool include_mutation_burden);

	const QList<QString>& cinChromosomes() const;
	///Setter for CIN chromosomes, pass argument in the form {"chr1", "chr2", ...}
	void setCinChromosomes(const QList<QString>& chromosomes);

	bool fusionsDetected() const;
	void setFusionsDetected(bool detected);

	QString tmbReferenceText() const;
	void setTmbReferenceText(QString ref_text);

	QStringList quality() const;
	void setQuality(QStringList qual);

	QString limitations() const;
	void setLimitations(QString limitations);

	QString filterName() const;
	void setFilterName(QString filter);

	FilterCascade filters() const;
	void setFilters(FilterCascade filters);

	int cnvLohCount() const;
	void setCnvLohCount(int cnv_loh_count);

	int cnvTaiCount() const;
	void setCnvTaiCount(int cnv_tai_count);

	int cnvLstCount() const;
	void setCnvLstCount(int cnv_lst_count);

	QString hrdStatement() const;
	void setHrdStatement(const QString& hrd_statement);

	double ploidy() const;
	void setPloidy(double ploidy);

	QDate evaluationDate() const;
	void setEvaluationDate(QDate date);

private:
	QList<SomaticReportVariantConfiguration> variant_config_;

	QList<SomaticReportGermlineVariantConfiguration> germ_variant_config_;

	QString created_by_;
	QDateTime created_at_;
	QString target_region_name_;

	bool include_tum_content_clonality_;
	bool include_tum_content_snp_af_;
	bool include_tum_content_histological_;
	bool include_tum_content_estimated_;

	double tum_content_estimated_;

	bool include_msi_status_;
	bool include_cnv_burden_;
	bool include_mutation_burden_;
	double ploidy_;

	QString hrd_statement_;

	//CNV metrics
	int cnv_loh_count_; //loss of heterocygosity
	int cnv_tai_count_; //telomeric allelic imbalance
	int cnv_lst_count_; //long state transition

	//list of instable chromosomes in the form {"chr1", "chr2" ....}
	QList<QString> cin_chromosomes_;

	QString limitations_;

	bool fusions_detected_;

	//Reference text for tumor mutation burden
	QString tmb_reference_text_;

	//Quality description
	QStringList quality_;

	//Name of filter
	QString filter_name_;
	//filters
	FilterCascade filters_;

	//Date when evaluation was performed
	QDate evaluation_date_;
};

#endif // SOMATICREPORTCONFIGURATION_H
