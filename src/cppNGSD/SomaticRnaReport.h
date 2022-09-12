#ifndef SOMATICRNAREPORT_H
#define SOMATICRNAREPORT_H

#include <QMultiMap>
#include "RtfDocument.h"
#include "NGSD.h"
#include "FilterCascade.h"
#include "VariantList.h"
#include "SomaticCnvInterpreter.h"
#include "SomaticReportSettings.h"


struct CPPNGSDSHARED_EXPORT SomaticRnaReportData : public SomaticReportSettings
{
	//copy constructor that initializes base members from SomaticReportSettings
	SomaticRnaReportData(const SomaticReportSettings& other);

	QString rna_ps_name;
	QString rna_fusion_file;
	QString rna_bam_file;
	QString ref_genome_fasta_file;

	//path to RNA expression file
	QString rna_expression_file;

	//reference tissue which was used for HPA annotation
	QString rna_hpa_ref_tissue;


	//list for fusions pics containing hex png data, width and height
	QList<RtfPicture> fusion_pics;

	QList<RtfPicture> expression_plots;

	//QCML data of RNA
	QCCollection rna_qcml_data;

	//Correlation of expression to cohort
	double expression_correlation;

	int cohort_size = 0;
};



class CPPNGSDSHARED_EXPORT SomaticRnaReport
{
public:
	SomaticRnaReport(const VariantList& snv_list, const CnvList& cnv_list, const SomaticRnaReportData& data);

	///write RTF to file
	void writeRtf(QByteArray out_file);

	//struct holding data from arriba fusion input file
	struct arriba_sv
	{
		QByteArray gene_left;
		QByteArray gene_right;

		QByteArray transcipt_left;
		QByteArray transcipt_right;

		QByteArray breakpoint_left;
		QByteArray breakpoint_right;

		QByteArray type;
	};

	///Checks whether all annotations neccessary for creating an RNA report are available
	static bool checkRequiredSNVAnnotations(const VariantList& variants);
	///Checks whether all annotations neccessary for creating RNA CNV table are available
	static bool checkRequiredCNVAnnotations(const CnvList& cnvs);

	///Calculates a rank for expression  (=1 high or =2 low) depending on gene expression and gene role
	static int rank(double tpm, double mean_tpm, SomaticGeneRole::Role gene_role);

private:
	NGSD db_;

	const SomaticRnaReportData& data_;

	//Somatic DNA SNVs
	VariantList dna_snvs_;
	//Somatic DNA CNVs
	CnvList dna_cnvs_;
	//Somatic RNA fusions
	QList<arriba_sv> svs_;

	struct ExpressionData
	{
		QByteArray symbol;

		QByteArray pathway;

		SomaticGeneRole role; //to be determined from somatic gene role

		//expression in tumor
		double tumor_tpm = std::numeric_limits<double>::quiet_NaN();

		//reference tpm from human protein atlas HPA
		double hpa_ref_tpm = std::numeric_limits<double>::quiet_NaN();

		//cohort mean tpm
		double cohort_mean_tpm = std::numeric_limits<double>::quiet_NaN();

		//log2fold change between log sample tpm and log cohort tpm
		double log2fc = std::numeric_limits<double>::quiet_NaN();

		//pvalue for log2fc between cohort and sample
		double pvalue = std::numeric_limits<double>::quiet_NaN();
	};


	//expression data per gene, will be filled only with genes that have a DNA variant
	QMap<QByteArray, ExpressionData> expression_per_gene_;

	//expression data per pathways, taken from NGSD
	QList<ExpressionData> pathways_;

	//expression data for genes with a pvalue < 0.05
	QList<ExpressionData> high_confidence_expression_;


	///Creates table that containts fusions from RNA data
	RtfTable partFusions();
	///Creates table with structural variants;
	RtfTable partSVs();
	///Creates block with fusion pictures from arriba
	RtfSourceCode partFusionPics();
	///Creates block with gene expression pictures
	RtfSourceCode partExpressionPics();
	///Creates table that contains expression of detected somatic variants from DNA/RNA data
	RtfTable partSnvTable();
	///Creates table that contains CN altered genes and their TPM
	RtfTable partCnvTable();
	///Creates explanation text for SNV and CNV table
	RtfParagraph partVarExplanation();
	///Gene expression table of pre-selected  genes
	RtfTable partGeneExpression();
	///Creates explanation text for SNV and CNV table
	RtfParagraph partGeneExprExplanation();
	//Creates snv table with uncertain (class 3) Variants:
	RtfTable uncertainSnvTable();

	///Creates a table that contains the top 10 of the strongest altered genes
	RtfSourceCode partTop10Expression();

	///general information
	RtfTable partGeneralInfo();

	///Returns TPM from annotation field, orig. entry has the form gene1=0.00,gene2=2.21,gene3=..., if not found it returns -1.
	double getRnaData(QByteArray gene, QString field, QString key);
	///Translates reference tissue type into German
	RtfSourceCode trans(QString orig_entry, int font_size=-1) const;

	///Formats double to float, in case it fails to "n/a"
	RtfSourceCode formatDigits(double in, int digits=0);


	RtfSourceCode expressionChange(const ExpressionData& data);

	RtfDocument doc_;

};

#endif // SOMATICRNAREPORT_H
