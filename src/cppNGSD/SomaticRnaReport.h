#ifndef SOMATICRNAREPORT_H
#define SOMATICRNAREPORT_H

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

	//path to RNA expression counts.tsv file.
	QString rna_counts_file;
	QString rna_stats_file;

	//list for fusions pics containing hex png data, width and height
	QList<std::tuple<QByteArray,int,int>> fusion_pics;

	QList<std::tuple<QByteArray,int,int>> expression_plots;

	//QCML data of RNA
	QCCollection rna_qcml_data;

	//Correlation of expression to cohort
	double expression_correlation;
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

	///Returns reference tissue, resolved from variant list variants
	static QString refTissueType(const VariantList& variants);

	///Checks whether reference tissue is unique in NGSD and is same as in GSVar file
	void checkRefTissueTypeInNGSD(QString ref_type, QString tumor_dna_ps_id);

	///Calculates a rank for CNVs (=1 high or =2 low) depending on gene expression and gene role
	static int rankCnv(double tpm, double mean_tpm, SomaticGeneRole::Role gene_role);

private:
	NGSD db_;

	const SomaticRnaReportData& data_;

	//Somatic DNA SNVs
	VariantList dna_snvs_;
	//Somatic DNA CNVs
	CnvList dna_cnvs_;
	//Somatic RNA fusions
	QList<arriba_sv> svs_;

	struct pathway_info
	{
		QByteArray gene;
		QByteArray pathogenicity;
		QByteArray pathway;

		double tumor_tpm;
		double ref_tpm;
	};

	QMap<QByteArray, pathway_info> pathway_infos_;

	///Creates table that containts fusions from RNA data
	RtfTable partFusions();
	///Creates table with structural variants;
	RtfTable partSVs();

	RtfSourceCode partFusionPics();
	RtfSourceCode partExpressionPics();

	///Creates table that contains expression of detected somatic variants from DNA/RNA data
	RtfTable partSnvTable();
	///Creates table that contains CN altered genes and their TPM
	RtfTable partCnvTable();
	///Creates explanation text for SNV and CNV table
	RtfParagraph partVarExplanation();

	///Gene expression table of pre-selected  genes
	RtfTable partGeneExpression();

	///general information
	RtfTable partGeneralInfo();

	//returns RtfPicture from tuple with PNG data <QByteArrayhex,int,int>
	RtfPicture pngToRtf(std::tuple<QByteArray,int,int> tuple, int width_goal);

	///Returns TPM from annotation field, orig. entry has the form gene1=0.00,gene2=2.21,gene3=..., if not found it returns -1.
	double getRnaData(QByteArray gene, QString field, QString key);
	///Translates reference tissue type into German
	RtfSourceCode trans(QString orig_entry) const;

	///Formats tpm string which is annotated to GSVAR file
	RtfSourceCode formatDigits(QByteArray in, int digits=1);

	RtfDocument doc_;

};

#endif // SOMATICRNAREPORT_H
