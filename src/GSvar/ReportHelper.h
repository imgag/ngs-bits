#ifndef REPORTHELPER_H
#define REPORTHELPER_H

#include "VariantList.h"
#include "GeneSet.h"
#include "ClinCnvList.h"
#include "CnvList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Settings.h"
#include "QCCollection.h"
#include "OntologyTermCollection.h"
#include "FilterCascade.h"
#include <QDialog>
#include <QMultiMap>
#include <QDir>

///provides methods for generating rtf file
class RtfTools
{
public:
	///specifies the header of an RTF file
	static void writeRtfHeader(QTextStream& stream);
	///generates the header of a single Rtf row
	static void writeRtfTableSingleRowSpec(QTextStream& stream,const QList<int>& col_widths, bool border,bool shaded = false);
	///generater the header of a single Rtf row, QList borders specifies the border widths for each cell beginning from top,right,bottom,left
	static void writeRtfTableSingleRowSpec(QTextStream &stream, const QList<int> &col_widths, QList<int> borders);
	///generates a RTF table
	static void writeRtfWholeTable(QTextStream& stream, const QList< QList<QString> >& table, const QList<int>& col_widths, int font_size, bool border, bool bold);
	///generates a single row for an RTF table
	static void writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths, int font_size, bool border, bool bold);
	///generates a single row for an RTF table, QList borders specifies the border widths for each cell beginning from top,right,bottom,left
	static void writeRtfRow(QTextStream& stream, const QList<QString>& columns, const QList<int>& col_widths,int font_size, QList<int> borders,bool bold);

};

///representation of CGI information of a drug reported by CGI
class CGIDrugReportLine
{
	friend class CGIDrugTable;
public:
	CGIDrugReportLine();

	///two CGI lines are equal if the gene, drug, tumor entity and effect are equal
	bool operator==(const CGIDrugReportLine& rhs) const
	{
		if(this->drug() != rhs.drug() && this->entity() != rhs.entity() && this->effect() != rhs.effect() && this->gene() != rhs.gene()) return false;
		return true;
	}

	/// entitites are compared according gene name
	bool operator<(const CGIDrugReportLine& rhs) const
	{
		if(QString::compare(gene_,rhs.gene(),Qt::CaseInsensitive) < 0) return true;
		return false;
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

	void setEvidence(QString evid)
	{
		evidence_ = evid;
	}

	void setSource(QString source)
	{
		source_ = source;
	}

	void setEntity(QString entity)
	{
		entity_ = entity;
	}

	///returns amino acid change in 3-letters-code if nomenclature in alteration_type was recognized.
	///returns "AMP" and "DEL" in case of CNVs
	static const QString proteinChange(QString aa_change);

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

	const QList<CGIDrugReportLine> drugsSortedPerGeneName() const;

	///Get CGI drug report from file
	void load(const QString& file_name);

	///Remove drugs if they already occured in evidence level 1
	void removeDuplicateDrugs();

	///Merges duplicates which occur in the same evidence level
	void mergeDuplicates(int evid_level);

	const QList<CGIDrugReportLine> values() const
	{
		return drug_list_.values();
	}

	int count()
	{
		return drug_list_.count();
	}

private:
	QMultiMap <int,CGIDrugReportLine> drug_list_;
};

///creates a somatic RTF report
class ReportHelper
{
public:
	ReportHelper();
	ReportHelper(QString snv_filename, const ClinCnvList& filtered_cnvs, const FilterCascade& filters, const QString& target_region="");
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
	///transforms GSVar coordinates of Variants to VCF INDEL-standard
	VariantList gsvarToVcf(const VariantList& gsvar_list, const QString& orig_name);

	///returns best matching transcript - or an empty transcript
	VariantTranscript selectSomaticTranscript(const Variant& variant);

	void writeGapStatistics(QTextStream &stream, const QString& target_file);
	QHash<QByteArray, BedFile> gapStatistics(const BedFile& region_of_interest);

	///Writes Rtf table containing given snvs
	void writeSnvList(QTextStream& stream, const QList<int>& col_widths, const VariantList& snvs);
	///Writes Rtf table containing CNVs per gene
	void writeCnvGeneList(QTextStream& stream, const QList<int>& col_widths, const GeneSet& target_genes);
	///generates table with CNVs
	void writeCnvList(QTextStream& stream, const QList<int>& colWidths);

	///writes table with drug annotation
	void writeRtfCGIDrugTable(QTextStream& stream, const QList<int>& col_widths);

	///SNV file
	QString snv_filename_;

	///target region
	QString target_region_ = "";

	///Germline filenames;
	QString germline_snv_filename_;

	///path to CGI drug annotation file
	QString cgi_drugs_path_;
	///path to germline SNV file
	QDir germline_snv_path_;

	///path to MANTIS file (microsatellite instabilities)
	QString mantis_msi_path_;
	double mantis_msi_swd_value_;

	///Sequence ontology that contains the SO IDs of coding and splicing transcripts
	OntologyTermCollection obo_terms_coding_splicing_;

	///tumor ID
	QString tumor_id_;
	///normal ID
	QString normal_id_;

	///Input VariantList
	VariantList snv_variants_;

	///VariantList for relevant germline SNVs
	VariantList snv_germline_;

	///CNVList for input (filtered) variants
	ClinCnvList cnvs_filtered_;

	NGSD db_;

	QCCollection qcml_data_;

	///Processing system data
	ProcessingSystemData processing_system_data;

	///CGI cancer acronym (extracted from .GSVar file)
	QString cgi_cancer_type_;

	///ICD10 text diagnosis tumor
	QString icd10_diagnosis_code_;
	///tumor fraction according genlab
	QString histol_tumor_fraction_;

	double mutation_burden_;

	///indices for somatic variant file
	int snv_index_cgi_driver_statement_;
	int snv_index_cgi_gene_role_;
	int snv_index_cgi_transcript_;
	int snv_index_coding_splicing_;
	int snv_index_cgi_gene_;

	///indices for somatic CNV file
	int cnv_index_cgi_gene_role_;
	int cnv_index_cnv_type_;
	int cnv_index_cgi_genes_;
	int cnv_index_cgi_driver_statement_;

	///Filter list
	FilterCascade filters_;
};

#endif // REPORTHELPER_H
