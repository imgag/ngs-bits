#ifndef SOMATICREPORTSETTINGS_H
#define SOMATICREPORTSETTINGS_H

#include "SomaticReportConfiguration.h"
#include "FilterCascade.h"
#include "NGSD.h"
#include "VariantList.h"

struct CPPNGSDSHARED_EXPORT SomaticReportSettings
{
	///Default constructor
	SomaticReportSettings();

	SomaticReportConfiguration report_config;
	QString tumor_ps;
	QString normal_ps;

	QString msi_file;
	QString viral_file;

	QString sbs_signature;
	QString id_signature;
	QString dbs_signature;
	QString cnv_signature;

	//ICD10 diagnosis
	QString icd10;

	//Clinical phenotype
	QString phenotype;

	//hex representation of IGV snapshot
	QByteArray igv_snapshot_png_hex_image;
	//width of IGV snapshot in pixels
	int igv_snapshot_width;
	//height of IGV snapshot in pixels
	int igv_snapshot_height;

	QMap<QByteArray, QByteArrayList> preferred_transcripts;

	//Sequence ontology that contains the SO IDs of coding and splicing transcripts
	OntologyTermCollection obo_terms_coding_splicing;


	TargetRegionInfo target_region_filter;

	//parse msi-file and return the stepwise difference value
	double get_msi_value();

	///returns variant list according filters and include/exclude report_config settings. Include from settings will overwrite FilterCascade entry
	static VariantList filterVariants(const VariantList& snvs, const SomaticReportSettings& sett, bool throw_errors=true);
	///returns cnv list according excluded cnvs from somatic report_config settings
	static CnvList filterCnvs(const CnvList& cnvs, const SomaticReportSettings& sett);
	///returns list containing germline variants.
	static VariantList filterGermlineVariants(const VariantList& germl_snvs, const SomaticReportSettings& sett);
};

#endif // SOMATICREPORTSETTINGS_H
