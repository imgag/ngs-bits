#ifndef SOMATICCFDNAREPORT_H
#define SOMATICCFDNAREPORT_H

#include "RtfDocument.h"
#include "NGSD.h"
#include "SomaticReportSettings.h"
#include "VariantHgvsAnnotator.h"



struct CPPNGSDSHARED_EXPORT SomaticcfDNAReportData : public SomaticReportSettings
{
	//copy constructor that initializes base members from SomaticReportSettings
	SomaticcfDNAReportData(const SomaticReportSettings& other, const CfdnaDiseaseCourseTable& table_data);

	CfdnaDiseaseCourseTable table;

	//QCML data of RNA
	QList<QCCollection> cfDNA_qcml_data;

};

struct CodingSplicingAnno
{
	Transcript trans;
	VariantConsequence consequence;
};

class CPPNGSDSHARED_EXPORT SomaticcfDnaReport
{
public:
	SomaticcfDnaReport(const SomaticcfDNAReportData& data);

	///write RTF to file
	void writeRtf(QByteArray out_file);


private:
	NGSD db_;
	SomaticcfDNAReportData data_;
	RtfDocument doc_;

	RtfTable partSnvTable(int cfdna_idx_start, int cfdna_idx_end);
	RtfParagraph partSnvExplanation();
	RtfTable partGeneralGeneticTable();
	RtfTable partGeneralInfo();

	///Formats double to float, in case it fails to "n/a"
	RtfSourceCode formatDigits(double in, int digits=0);
	QByteArray cleanConsequenceString(QByteArray consequence);
	CodingSplicingAnno getPreferedCodingAndSplicing(const VcfLine& variant);
	QByteArray getMrdTableValue(const QByteArray& type, int cfdna_idx);

};

#endif // SOMATICRNAREPORT_H
