#ifndef SOMATICXMLREPORTGENERATOR_H
#define SOMATICXMLREPORTGENERATOR_H

#include "cppNGSD_global.h"
#include "NGSD.h"
#include "GeneSet.h"
#include "GenLabDB.h"
#include "SomaticReportConfiguration.h"
#include "SomaticReportSettings.h"
#include <QDate>

///Container for XML data exchange format
struct CPPNGSDSHARED_EXPORT SomaticXmlReportGeneratorData
{
	SomaticXmlReportGeneratorData(const SomaticReportSettings& som_settings,const VariantList& snvs, const VariantList& germl_snvs, const CnvList& cnvs);

	const SomaticReportSettings& settings;

	const VariantList& tumor_snvs;
	const VariantList& germline_snvs;
	const CnvList& tumor_cnvs;

	double tumor_content_histology;
	double tumor_content_snvs;
	double tumor_content_clonality;

	double tumor_mutation_burden;
	double mantis_msi;

	//processing system information
	BedFile processing_system_roi;
	GeneSet processing_system_genes;

	//Check whether all neccessary data is set up consistently
	void check() const;
};

///Container for XML data exchange format
class CPPNGSDSHARED_EXPORT SomaticXmlReportGenerator
{
public:
	SomaticXmlReportGenerator();

	static QString generateXML(const SomaticXmlReportGeneratorData &data, NGSD& db, bool test=false);

	static void checkSomaticVariantAnnotation(const VariantList& vl);

private:
	static void generateXML(const SomaticXmlReportGeneratorData& data, QString& output, NGSD& db, bool test=false);

	//validates somatic xml report against .xsd file in resources dir
	static void validateXml(const QString& xml);
};

#endif // SOMATICXMLREPORTGENERATOR_H
