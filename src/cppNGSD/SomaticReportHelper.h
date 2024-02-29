#ifndef SomaticReportHelper_H
#define SomaticReportHelper_H

#include "VariantList.h"
#include "GeneSet.h"
#include "CnvList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Settings.h"
#include "QCCollection.h"
#include "TSVFileStream.h"
#include "OntologyTermCollection.h"
#include "FilterCascade.h"
#include <QMultiMap>
#include <QDir>
#include "RtfDocument.h"
#include "BedpeFile.h"
#include "SomaticReportSettings.h"
#include "HttpRequestHandler.h"
#include "SomaticXmlReportGenerator.h"

struct CPPNGSDSHARED_EXPORT SomaticVirusInfo
{
	QByteArray chr;
	int start;
	int end;
	QByteArray name;
	int reads;
	double coverage;
	int mismatches;
	double idendity;

	///Virus gene, extracted from name_ if possible
	QByteArray virusGene() const
	{
		QByteArray gene_name = name;
		if(name.split('_').count() > 1)
		{
			QByteArray reduced_name = name.split('_').at(0) + "_";
			gene_name = gene_name.replace(reduced_name,"");
			return gene_name;
		}
		return "";
	}

	QByteArray virusName() const
	{
		QByteArray virus_name = "";
		if(name.split('_').count() > 0)
		{
			virus_name = name.split('_').at(0);
		}
		return virus_name;
	}
};

struct CPPNGSDSHARED_EXPORT HlaLine
{
	QByteArray bam_file;
	QByteArray sample;
	QByteArray ethnicity;
	QByteArray gene;
	QByteArray allele1;
	QByteArray allele2;
	float precision;
	bool passed;
	int depth_allele1;
	int depth_allele2;
};

struct CPPNGSDSHARED_EXPORT SomaticHlaInfo
{
	QList<HlaLine> lines;

	SomaticHlaInfo(QString hla_file)
	{
		if (! VersatileFile(hla_file).exists()) return;

		TSVFileStream hla_stream(hla_file);
		while (!hla_stream.atEnd())
		{
			bool float_ok;
			bool depth1_ok;
			bool depth2_ok;

			QByteArrayList values = hla_stream.readLine();

			if (values.count() != 11)
			{
				THROW(FileParseException, "HLA file '" + hla_file + "' contained a line string with an unexpected number of columns! Column values: " + values.join(","));
			}

			HlaLine hla;
			hla.bam_file =	values[0];
			hla.sample =	values[1];
			hla.ethnicity = values[2];
			hla.gene =		values[3];
			hla.allele1 =	values[4];
			hla.allele2 =	values[5];
			hla.precision =	values[6].toFloat(&float_ok);
			hla.passed =	values[7] == "Pass";
			hla.depth_allele1 =	values[8].toInt(&depth1_ok);
			hla.depth_allele2 =	values[9].toInt(&depth2_ok);

			if (! (float_ok && depth1_ok && depth2_ok))
			{
				THROW(FileParseException, "HLA file '" + hla_file + "' contained a unexpected string in a number column (pval, a1_reads or a2_reads)!");
			}

			lines.append(hla);
		}
	}

	bool isValid()
	{
		return this->lines.count() != 0;
	}

	QByteArray getGeneAllele(QString gene, bool allele1)
	{
		foreach(HlaLine line, this->lines)
		{
			if (line.gene == gene)
			{
				if (allele1)
				{
					return line.allele1;
				}
				else
				{
					return line.allele2;
				}
			}
		}
		THROW(ArgumentException, "Given Gene not found in HLA lines: " + gene);
	}

};

///creates a somatic RTF report
class CPPNGSDSHARED_EXPORT SomaticReportHelper
{
public:
	///Constructor loads data into class
	SomaticReportHelper(GenomeBuild build, const VariantList& variants, const CnvList &cnvs, const VariantList& germline_variants, const SomaticReportSettings& settings, bool test_db=false);

	///write Rtf File
	void storeRtf(const QByteArray& out_file);

	///write XML file
	void storeXML(QString file_name);

	///methods that create TSV files for QBIC
	void storeQbicData(QString path);

	///Returns CNV type, e.g. DEL (het) according copy number
	static QByteArray CnvTypeDescription(int tumor_cn, bool add_cn);
	///Returns true if germline variant file is valid (annotations and at least one germline variant)
	static bool checkGermlineSNVFile(const VariantList& germline_variants);

	///Returns maximum tumor clonailty in cnv file
	static double getCnvMaxTumorClonality(const CnvList& cnvs);

	double getTumorContentBySNVs();

	///Returns CNV burden, i.e. total cnv size divided by genome size in %
	static double cnvBurden(const CnvList& cnvs)
	{
		return cnvs.totalCnvSize() / 3101788170. * 100;
	}

	static QString trans(const QString& text);

	static QByteArray trans(QByteArray text)
	{
		return trans( QString(text) ).toUtf8();
	}

	///returns best matching transcript - or an empty transcript
	static VariantTranscript selectSomaticTranscript(NGSD& db, const Variant& variant, const SomaticReportSettings& settings, int index_co_sp);

	///adds necessary colors to the to the RTF document
	static void addColors(RtfDocument& doc);


	//functions for testing functionality:
	double getHistTumorContent()
	{
		return histol_tumor_fraction_;
	}

	double getTumorMutationBurden()
	{
		return mutation_burden_;
	}

	double getMsiValue()
	{
		return mantis_msi_swd_value_;
	}

	SomaticXmlReportGeneratorData getXmlData(const VariantList& som_var_in_normal);


private:
	///transforms GSVar coordinates of Variants to VCF INDEL-standard
	VariantList gsvarToVcf(const VariantList& gsvar_list, const QString& orig_name);

	///Parses CN to description
	RtfSourceCode CnvDescription(const CopyNumberVariant& cnv, const SomaticGeneRole& role, double snv_tumor_af=-1);

	///Parses annotated cytobands to text, "" if not annotation available
	QByteArray cytoband(const CopyNumberVariant& cnv);

	RtfTableRow overlappingCnv(const CopyNumberVariant& cnv, QByteArray gene, const QList<int>& col_widths, double snv_tumor_af);

	///parts of the report
	///Generates table with genral information
	RtfSourceCode partSummary();
	///Generates table with classified most relevant variants
	RtfSourceCode partRelevantVariants();
	///Generates table with variants of unknown significance
	RtfSourceCode partUnclearVariants();
	///Generates table with chromosomal aberratons
	RtfSourceCode partCnvTable();
	///Generates table for fusions
	RtfSourceCode partFusions();
	///generates table with germline SNPs
	RtfSourceCode partPharmacoGenetics();
	///generates meta data (e.g. coverage, depth)
	RtfSourceCode partMetaData();
	///generates pathway table
	RtfSourceCode partPathways();
	///generates with billing informationen (gene names in first table + OMIM numbers) for health funds according EBM
	RtfSourceCode partBillingTable();
	///Generates table with virus information
	RtfSourceCode partVirusTable();
	///Generates part with somatic IGV snapshot
	RtfSourceCode partIgvScreenshot();
	///creates table with SNVs, relevant germline SNPs (class 4/5) and overlapping CNVs
	RtfTable snvTable(const QSet<int>& indices, bool high_impact_table=true);
	///creates a table row of the given variant for the SNV table.
	RtfTableRow snvRow(const Variant& snv, const VariantTranscript& transcript, const QList<int>& col_widths);
	///get filepath for HLA file
	QString getHlaFilepath(QString ps_name);
	///creates table with hla_genotyper information
	RtfTable hlaTable(QString ps_tumor, QString ps_normal);
	///creates table with hla_genotyper information
	RtfTable signatureTable();

	void signatureTableHelper(RtfTable& table, QString file, const QMap<QByteArray, QByteArray>& descriptions, const QByteArray& type);

	QByteArray prepareTranscriptType(QByteArray transcript_type);
	double getTumorContentBioinf();

	//skipped amplifications in somaticalterationtable
	GeneSet skipped_amp_ = {};

	//Somatic SNVs/INDELs
	VariantList somatic_vl_;
	QSet<int> somatic_vl_high_impact_indices_; //small variants with high impact i.e. they are added to the pathway list in bold
	QSet<int> somatic_vl_low_impact_indices_; //small variants with low impact i.e. they are added to the pathway list, but not bold

	QSet<int> low_impact_indices_converted_to_high_; // small variants with low impact, in the same gene as a high impact variant. (variants that were moved to the high impact table).

	GenomeBuild build_;

	const SomaticReportSettings& settings_;
	//VariantList for relevant germline SNVs
	const VariantList& germline_vl_;

	//Microsatellite instability MANTIS step-wise-difference metric
	double mantis_msi_swd_value_;

	//CNVList for somatic (filtered) copy-number altered variants
	CnvList cnvs_;
	QHash<int, GeneSet> cnv_high_impact_indices_; //CNVs with high impact (i.e. they are added to the pathway list): CNV index => gene list

	//Somatic viruses (original file usually in tumor dir)
	QList<SomaticVirusInfo> validated_viruses_;

	NGSD db_;

	QCCollection tumor_qcml_data_;
	QCCollection normal_qcml_data_;

	//Processing system data
	ProcessingSystemData processing_system_data_;

	//genes that are printed on last report page for EBM billing. List is to be filled in snvTable()
	GeneSet ebm_genes_;

	//ICD10 text diagnosis tumor
	QString icd10_diagnosis_code_;
	//histologic tumor fraction according genlab
	double histol_tumor_fraction_;

	double mutation_burden_;

	//indices for somatic variant file
	int snv_index_coding_splicing_;


	//indices for somatic CNV file
	int cnv_index_cn_change_;
	int cnv_index_cnv_type_;
	int cnv_index_state_;
	int cnv_index_tumor_clonality_;
	int cnv_index_cytoband_;

	RtfDocument doc_;

	void saveReportData(QString filename, QString path, QString content);

	void somaticSnvForQbic(QString path);
	void germlineSnvForQbic(QString path);
	void somaticCnvForQbic(QString path);
	void germlineCnvForQbic(QString path);
	void somaticSvForQbic(QString path);
	void metaDataForQbic(QString path);
};

#endif // SomaticReportHelper_H
