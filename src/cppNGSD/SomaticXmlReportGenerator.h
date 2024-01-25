#ifndef SOMATICXMLREPORTGENERATOR_H
#define SOMATICXMLREPORTGENERATOR_H

#include "cppNGSD_global.h"
#include "NGSD.h"
#include "GeneSet.h"
#include "SomaticReportConfiguration.h"
#include "SomaticReportSettings.h"
#include "RtfDocument.h"
#include <QDate>
#include <QXmlStreamWriter>

///Container for XML data exchange format
struct CPPNGSDSHARED_EXPORT SomaticXmlReportGeneratorData
{
	SomaticXmlReportGeneratorData(GenomeBuild genome_build, const SomaticReportSettings& som_settings, const VariantList& snvs, const VariantList& germl_snvs, const CnvList& cnvs);

	GenomeBuild build;
	const SomaticReportSettings& settings;

	const VariantList& tumor_snvs;
	const VariantList& germline_snvs;
	const CnvList& tumor_cnvs;

	double tumor_content_histology;
	double tumor_content_snvs;
	double tumor_content_clonality;
	double tumor_content_estimated;

	double tumor_mutation_burden;
	double mantis_msi;

	RtfSourceCode rtf_part_header;
	RtfSourceCode rtf_part_footer;
	RtfSourceCode rtf_part_summary;
	RtfSourceCode rtf_part_relevant_variants;
	RtfSourceCode rtf_part_unclear_variants;
	RtfSourceCode rtf_part_cnvs;
	RtfSourceCode rtf_part_svs;
	RtfSourceCode rtf_part_pharmacogenetics;
	RtfSourceCode rtf_part_general_info;
	QByteArray rtf_part_igv_screenshot;
	RtfSourceCode rtf_part_mtb_summary;
	RtfSourceCode rtf_part_hla_summary;


	//Check whether all neccessary data is set up consistently
	void check() const;
};

///Container for XML data exchange format
class CPPNGSDSHARED_EXPORT SomaticXmlReportGenerator
{
public:
	SomaticXmlReportGenerator();

	static void checkSomaticVariantAnnotation(const VariantList& vl);

	static void generateXML(const SomaticXmlReportGeneratorData& data, QSharedPointer<QFile> out_file, NGSD& db, bool test=false);

	///writes a ReportDocumentParts element
	static void writeReportPartsElement(QXmlStreamWriter& w, QString name, RtfSourceCode rtf_part);


};

#endif // SOMATICXMLREPORTGENERATOR_H
