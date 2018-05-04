#ifndef REPORTHELPER_H
#define REPORTHELPER_H

#include "VariantList.h"
#include "GeneSet.h"
#include "CnvList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Settings.h"
#include "QCCollection.h"
#include "OntologyTermCollection.h"
#include <QDialog>
#include <QMultiMap>

///provides methods for generating rtf file
class RtfTools
{
public:
	///specifies the header of an RTF file
	static void writeRtfHeader(QTextStream& stream);
	///generates the header of a single Rtf row
	static void writeRtfTableSingleRowSpec(QTextStream& stream,const QList<int>& col_widths, bool border);
	///generates a RTF table
	static void writeRtfWholeTable(QTextStream& stream, const QList< QList<QString> >& table, const QList<int>& col_widths, int font_size, bool border, bool bold);
	///generates a single row for an RTF table
	static void writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths, int font_size, bool border, bool bold);
};

///representation of CGI information of a drug reported by CGI
class CGIDrugReportLine
{
	friend class CGIDrugTable;
public:
	CGIDrugReportLine();

	///two CGI lines are equal if the drug is equal
	bool operator==(const CGIDrugReportLine& rhs)
	{
		if(this->drug() != rhs.drug()) return false;
		return true;
	}
	const QString& gene() const
	{
		return gene_;
	}

	const QString& alterationType() const
	{
		return alteration_type_;
	}


	const QString& drug() const
	{
		return drug_;
	}

	const QString& effect() const
	{
		return effect_;
	}

	const QString& evidence() const
	{
		return evidence_;
	}

	const QString& entity() const
	{
		return entity_;
	}
	const QString& source() const
	{
		return source_;
	}

	const QList<QString> asStringList() const
	{
		QList<QString> result;
		result.append(gene_);
		result.append(alteration_type_);
		result.append(entity_);
		result.append(drug_);
		result.append(effect_);
		result.append(evidence_);
		result.append(source_);
		return result;
	}

private:
	///gene in which alteration was observed
	QString gene_;
	///type of the alteration
	QString alteration_type_;
	///entities for which the drug was tested
	QString entity_;
	///drug reported for alteration
	QString drug_;
	///drug resistant or responsive
	QString effect_;
	///evidence of drug effect in sense of Guidlines, case report...
	QString evidence_;
	///publications assigned to drug
	QString source_;
};

class CGIDrugTable
{
public:
	CGIDrugTable();

	QList<CGIDrugReportLine> drugsByEvidenceLevel(int evid_group) const
	{
		return drug_list_.values(evid_group);
	}

	///return drugs by evidence as string list
	const QList< QList<QString> > drugsByEvidAsString(int evid_group);

	///Get CGI drug report from file, cnv alterations which do occur in keep_cnv_genes will be discharged
	void load(const QString& file_name, GeneSet keep_cnv_genes);

	///Remove drugs if they already occured in evid level 1
	void removeDuplicateDrugs();

	const QList<CGIDrugReportLine> values() const
	{
		return drug_list_.values();
	}

	int count()
	{
		return drug_list_.count();
	}

	//returns a list with cancer acronyms appearing in the given evidence level
	const QList<QByteArray> getAcronyms(int evid_level) const;

private:
	QMultiMap <int,CGIDrugReportLine> drug_list_;
};

///creates a somatic RTF report
class ReportHelper
{
public:
	ReportHelper();
	ReportHelper(QString snv_filename, GeneSet cnv_keep_genes_filter, QString target_region);
	///write Rtf File
	void writeRtf(const QString& out_file);

	///methods that create files for QBIC
	void somaticSnvForQbic();
	void germlineSnvForQbic();
	void somaticCnvForQbic();
	void germlineCnvForQbic();
	void somaticSvForQbic();
	void metaDataForQbic();

private:
	///Filters snv_variants_ for SNVs with annotation from CancerGenomeInterpreter.org
	VariantList filterSnvForCGIAnnotation(bool filter_for_target_region=false);
	///Filters cnv_cariants_, CNVs with zScors < 5 and without genes in cnv_keep_genes_filter_ will be discarded
	CnvList filterCnv();

	///transforms GSVar coordinates of Variants to vcf standard
	VariantList gsvarToVcf();

	///writes table with drug annotation
	void writeRtfCGIDrugTable(QTextStream& stream, const QList<int>& col_widths);

	///generates table with important SNVs including mutational burden
	void writeRtfTableSNV(QTextStream& stream, const QList<int>& colWidths, bool display_germline_hint = true);

	///generates table with important CNVs
	void writeRtfTableCNV(QTextStream& stream, const QList<int>& colWidths);

	///make gap statistics, grouped by gene as QByteArray and regions as BedFile
	QHash<QByteArray, BedFile> gapStatistics();

	///Somatic filenames
	QString snv_filename_;
	QString cnv_filename_;
	///path to CGI drug annotation file
	QString cgi_drugs_path_;

	///Sequence ontology that contains the SO IDs of coding and splicing transcripts
	OntologyTermCollection obo_terms_coding_splicing_;

	///tumor ID
	QString tumor_id_;
	///normal ID
	QString normal_id_;

	///Input CnvList and VariantList
	CnvList cnv_variants_;
	VariantList snv_variants_;

	NGSD db_;

	///Target region
	BedFile roi_;
	///filename that contains the target region
	QString target_region_;
	///genes that are included in the target region
	GeneSet genes_in_target_region_;

	///genes that are checked for germline variants (those appear later in the explanation of the RTF report)
	GeneSet genes_checked_for_germline_variants_;

	QCCollection qcml_data_;

	///Geneset with genes to be kept for CNV report
	GeneSet cnv_keep_genes_filter_;

	///CGI cancer acronym (extracted from .GSVar file)
	QString cgi_cancer_type_;

	///ICD10 text diagnosis tumor
	QString icd10_diagnosis_text_;
	QString icd10_diagnosis_code_;
	///tumor fraction according genlab
	QString histol_tumor_fraction_;

	double mutation_burden_;

	///indices for variant files
	int snv_index_filter_;
	int snv_index_cgi_driver_statement_;
	int snv_index_cgi_gene_role_;
	int snv_index_cgi_transcript_;
	int snv_index_coding_splicing_;
	int snv_index_cgi_gene_;

	///indices for CNV files
	int cnv_index_cgi_gene_role_;
	int cnv_index_cnv_type_;
	int cnv_index_cgi_genes_;
	int cnv_index_cgi_driver_statement_;
};

#endif // REPORTHELPER_H
