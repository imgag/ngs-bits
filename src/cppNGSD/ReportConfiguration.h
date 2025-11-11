#ifndef REPORTCONFIGURATION_H
#define REPORTCONFIGURATION_H

#include "cppNGSD_global.h"
#include "VariantType.h"
#include "VariantList.h"
#include "CnvList.h"
#include "BedpeFile.h"
#include "RepeatLocusList.h"

class NGSD;

///Variant meta data for report.
struct CPPNGSDSHARED_EXPORT ReportVariantConfiguration
{
	///Default constructor
	ReportVariantConfiguration();
	///Returns if the variant is to be shown in the report
	bool showInReport() const;
	///Returns if the variant is valid. Writes validation error message into given string list.
	bool isValid(QStringList& errors, FastaFileIndex& ref_index);
	///Equality operator
	bool operator==(const ReportVariantConfiguration& rhs);
	///Equality operator
	bool operator!=(const ReportVariantConfiguration& rhs) { return !operator==(rhs); }

	//general data
	int id = -1;
	VariantType variant_type;
	int variant_index; //index of the variant in the variant/CNV/SV list

	QString report_type;
	bool causal;
	QString classification;
	QString inheritance;
	bool de_novo;
	bool mosaic;
	bool comp_het;
	bool exclude_artefact; //variant is an artefact
	bool exclude_frequency; //variant frequency is too high in NGSD, gnomAD or elsewhere
	bool exclude_phenotype; //variant/gene does not match genotype
	bool exclude_mechanism; //variant pathomechanism not matching
	bool exclude_hit2_missing; //second variant missing is recessive context
	bool exclude_gus; //gene of unknown significance
	bool exclude_used_other_var_type; //used other variant type
	bool exclude_other; //other reason
	QString comments; //comments of 1. evaluation
	QString comments2; //comments of 2. evaluation
	QString rna_info;

	//general function for manual curation
	bool isManuallyCurated() const;

	//manual curation of small variants
	QString manual_var;
	QString manual_genotype;
	bool manualVarIsValid(FastaFileIndex& ref_index, QString* error= nullptr) const;
	bool manualVarGenoIsValid() const;
	void updateVariant(Variant& v, FastaFileIndex& ref_index, int genotype_col_idx) const;

	//manual curation of CNVs
	QString manual_cnv_start;
	QString manual_cnv_end;
	QString manual_cnv_cn;
	QString manual_cnv_hgvs_type;
	QString manual_cnv_hgvs_suffix;
	bool manualCnvStartIsValid() const;
	bool manualCnvEndIsValid() const;
	bool manualCnvCnIsValid() const;
	void updateCnv(CopyNumberVariant& cnv, const QByteArrayList& annotation_headers, NGSD& db) const;

	//manual curation of SVs
	QString manual_sv_start;
	QString manual_sv_end;
	QString manual_sv_genotype;
	QString manual_sv_hgvs_type;
	QString manual_sv_hgvs_suffix;
	QString manual_sv_start_bnd;
	QString manual_sv_end_bnd;
	QString manual_sv_hgvs_type_bnd;
	QString manual_sv_hgvs_suffix_bnd;
	bool manualSvStartIsValid() const;
	bool manualSvEndIsValid() const;
	bool manualSvGenoIsValid() const;
	bool manualSvStartBndIsValid() const;
	bool manualSvEndBndIsValid() const;
	void updateSv(BedpeLine& sv, const QByteArrayList& annotation_headers, NGSD& db) const;

	//manual curation of REs
	QString manual_re_allele1;
	QString manual_re_allele2;
	bool manualReAllele1IsValid() const;
	bool manualReAllele2IsValid() const;
	void updateRe(RepeatLocus& re) const;

	//Returns options for 'type' (taken and cached from NGSD)
	static QStringList getTypeOptions();
	//Returns options for 'inheritance_mode' (taken and cached from NGSD)
	static QStringList getInheritanceModeOptions();
	//Returns options for 'class' (taken and cached from NGSD)
	static QStringList getClassificationOptions();
	//Returns options for 'rna_info' (taken and cached from NGSD)
	static QStringList getRnaInfoOptions();
};

///struct to handle other causal variants
struct CPPNGSDSHARED_EXPORT OtherCausalVariant
{
	int id = -1;
	QString coordinates;
	QString gene;
	QString type;
	QString inheritance;
	QString comment;
	QString comment_reviewer1;
	QString comment_reviewer2;

	bool isValid() const
	{
		if(type.trimmed().isEmpty()) return false;
		if(coordinates.isEmpty()) return false;
		if(inheritance.trimmed().isEmpty()) return false;
		return true;
	}
};


///Report configuration
class CPPNGSDSHARED_EXPORT ReportConfiguration
	: public QObject
{
	Q_OBJECT

public:
	ReportConfiguration();

	///Returns the report configuration for variants
	const QList<ReportVariantConfiguration>& variantConfig() const;
	///Returns indices of the matching variants .
	QList<int> variantIndices(VariantType type, bool only_selected, QString report_type = QString()) const;

	///Returns if a report configuration exists for the variant.
	bool exists(VariantType type, int index) const;
	///Returns the matching report configuration (throws an error if not found).
	const ReportVariantConfiguration& get(VariantType type, int index) const;
	///Returns other causal variant
	OtherCausalVariant otherCausalVariant();
	///Set other causal variant
	void setOtherCausalVariant(const OtherCausalVariant& causal_variant);
	///Sets the report configuration for the variant. Returns if it already existed.
	void set(const ReportVariantConfiguration& config);
	///Marks the matching configuration for deletion. Returns if a configuration was removed.
	void remove(VariantType type, int index);

	///Returns the list of all variants which should be deleted from the NGSD
	const QList<ReportVariantConfiguration>& variantsToDelete() const;
	///Removes all variants from the deletion list
	void clearDeletionList();
	///Returns db ids of a variants as set
	QSet<int> getVariantConfigIds(VariantType type);

	///Returns by who the report config was created.
	QString createdBy() const;
	///Sets the creator name.
	void setCreatedBy(QString user_name);

	///Returns when the report config was created.
	QDateTime createdAt() const;
	///Sets the creation time
	void setCreatedAt(QDateTime time);

	QString lastUpdatedBy() const;
	QDateTime lastUpdatedAt() const;

	QString finalizedBy() const;
	QDateTime finalizedAt() const;
	bool isFinalized() const;

	///Returns the number of report config entries (all types).
	int count() const
	{
		return variant_config_.count();
	}

	///Returns a creation/modification/finalization history description test.
	QString history() const;

	///Returns a variant summary description test.
	QString variantSummary() const;

signals:
	///Signal that is emitted when the report configuration was changed.
	void variantsChanged();

private:
	QList<ReportVariantConfiguration> variant_config_;
	QList<ReportVariantConfiguration> variants_to_delete_;
	OtherCausalVariant other_causal_variant_;

	QString created_by_;
	QDateTime created_at_;

	QString last_updated_by_;
	QDateTime last_updated_at_;

	QString finalized_by_;
	QDateTime finalized_at_;

	//sort by variant index
	void sortByPosition();

	friend class NGSD;
};

#endif // REPORTCONFIGURATION_H
