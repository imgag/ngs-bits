#ifndef CLINVARUPLOADDATA_H
#define CLINVARUPLOADDATA_H

#include "PhenotypeList.h"
#include "BedpeFile.h"
#include "VariantType.h"
#include "ReportConfiguration.h"
#include "NGSD.h"

enum class ClinvarSubmissionType
{
	SingleVariant,
	CompoundHeterozygous
};

//Datastructure for upload data
struct ClinvarUploadData
{
	//sample data
	int variant_id1;
	int variant_id2 = -1;
	int report_variant_config_id1;
	int report_variant_config_id2 = -1;
	ReportVariantConfiguration report_variant_config1;
	ReportVariantConfiguration report_variant_config2;
	QString processed_sample;

	//disease data
	QList<SampleDiseaseInfo> disease_info;
	QString affected_status;

	//phenotype data
	PhenotypeList phenos;

	//type data
	ClinvarSubmissionType submission_type = ClinvarSubmissionType::SingleVariant;
	VariantType variant_type1 = VariantType::INVALID;
	VariantType variant_type2 = VariantType::INVALID;

	//variant data
	Variant snv1;
	Variant snv2;
	CopyNumberVariant cnv1;
	int cn1;
	int ref_cn1 = 2;
	CopyNumberVariant cnv2;
	int cn2;
	int ref_cn2 = 2;
	BedpeLine sv1;
	BedpeLine sv2;
	GeneSet genes;

	//additional info for re-upload
	QString stable_id;
	int user_id = -1;
	int variant_publication_id = -1;
};
#endif // CLINVARUPLOADDATA_H
