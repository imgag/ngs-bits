#ifndef SOMATICRNAREPORT_H
#define SOMATICRNAREPORT_H

#include "RtfDocument.h"
#include "NGSD.h"
#include "FilterCascade.h"
#include "VariantList.h"


class SomaticRnaReport
{
public:
	SomaticRnaReport(const VariantList& snv_list, const FilterCascade& filters, const CnvList& cnv_list, QString rna_ps_name);

	///write RTF to file
	void writeRtf(QByteArray out_file);

	//struct holding data from fusion input file
	struct fusion
	{
		QByteArray genes;

		QByteArray transcipt_id_left;
		QByteArray transcipt_id_right;

		//amino acids
		QByteArray aa_left;
		QByteArray aa_right;

		QByteArray type;
	};

	///Checks whether all annotations neccessary for creating an RNA report are available
	static bool checkRequiredSNVAnnotations(const VariantList& variants);
	///Checks whether all annotations neccessary for creating RNA CNV table are available
	static bool checkRequiredCNVAnnotations(const CnvList& cnvs);

	///Returns reference tissue, resolved from variant list variants
	static QString refTissueType(const VariantList& variants);

	void checkRefTissueTypeInNGSD(QString ref_type, QString tumor_dna_ps_id);

private:
	//processed sample name of RNA sample
	QString rna_ps_name_ = "";

	NGSD db_;

	//Somatic DNA SNVs
	VariantList dna_snvs_;
	//Somatic DNA CNVs
	CnvList dna_cnvs_;
	//Somatic RNA fusions
	QList<fusion> fusions_;

	//Tissue type for RNA reference TPM in SNV list
	QString ref_tissue_type_ = "";

	///Creates table that containts fusions from RNA data
	RtfTable fusions();
	///Creates table that contains expression of detected somatic variants from DNA/RNA data
	RtfTable snvTable();
	///Creates table that contains CN altered genes and their TPM
	RtfTable cnvTable();
	///Returns TPM from annotation field, orig. entry has the form gene1=0.00,gene2=2.21,gene3=..., if not found it returns -1.
	double getTpm(QByteArray gene, QByteArray field);
	///Translates reference tissue type into German
	RtfSourceCode trans(QString orig_entry) const;

	RtfDocument doc_;

	QByteArray CgiDriverDescription(QByteArray raw_cgi_input)
	{
		QByteArray out = raw_cgi_input;

		if(raw_cgi_input.contains("predicted driver")) out = "Treiber (vorhergesagt)";
		else if(raw_cgi_input.contains("known in")) out = "Treiber (bekannt)";
		else out = "Unklare Bedeutung";

		return out;
	}

};

#endif // SOMATICRNAREPORT_H
