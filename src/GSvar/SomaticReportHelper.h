#ifndef SomaticReportHelper_H
#define SomaticReportHelper_H

#include "VariantList.h"
#include "GeneSet.h"
#include "ClinCnvList.h"
#include "CnvList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Settings.h"
#include "QCCollection.h"
#include "TSVFileStream.h"
#include "OntologyTermCollection.h"
#include "FilterCascade.h"
#include <QDialog>
#include <QMultiMap>
#include <QDir>
#include "RtfDocument.h"

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

	const QString& id() const
	{
		return id_;
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
	///unique ID which refers to SNP
	QString id_;
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


struct cgi_info
{
	QByteArray acronym;
	QByteArray parent;
	QByteArray def_english;
	QByteArray def_german;

	//create whole list from file
	static QList<cgi_info> load(const QByteArray& file_name)
	{
		TSVFileStream acronym_translations(file_name);
		int i_cgi_acronym = acronym_translations.colIndex("ID",true);
		int i_def_english = acronym_translations.colIndex("NAME",true);
		int i_def_german = acronym_translations.colIndex("NAME_GERMAN",true);

		QList<cgi_info> out;

		while(!acronym_translations.atEnd())
		{
			QByteArrayList current_line = acronym_translations.readLine();
			cgi_info temp;
			temp.acronym = current_line.at(i_cgi_acronym);
			temp.def_english = current_line.at(i_def_english);
			temp.def_german = current_line.at(i_def_german);
			out << temp;
		}
		return out;
	}
	bool operator==(const cgi_info& rhs) const
	{
		if(this->acronym != rhs.acronym) return false;
		return true;
	}
};

//struct holding reference data for tumor mutation burden (DOI:10.1186/s13073-017-0424-2)
struct tmb_info
{
	QByteArray hpoterm;
	int cohort_count;
	double tmb_median;
	double tmb_max;
	QByteArray tumor_entity;

	static QList<tmb_info> load(const QByteArray& file_name)
	{
		TSVFileStream in(file_name);

		int i_hpoterms = in.colIndex("HPO_TERMS",true);
		int i_count = in.colIndex("COUNT",true);
		int i_tmb_median = in.colIndex("TMB_MEDIAN",true);
		int i_tmb_max = in.colIndex("TMB_MAXIMUM",true);
		int i_tumor_entity = in.colIndex("TUMOR_ENTITY",true);

		QList<tmb_info> out;
		while(!in.atEnd())
		{
			QByteArrayList current_line = in.readLine();
			tmb_info tmp;
			tmp.hpoterm = current_line.at(i_hpoterms);
			tmp.cohort_count = current_line.at(i_count).toInt();
			tmp.tmb_median = current_line.at(i_tmb_median).toDouble();
			tmp.tmb_max = current_line.at(i_tmb_max).toDouble();
			tmp.tumor_entity = current_line.at(i_tumor_entity);
			out << tmp;
		}
		return out;
	}
	bool operator==(const tmb_info& rhs) const
	{
		if(this->hpoterm == rhs.hpoterm) return true;
		return true;
	}
};

///creates a somatic RTF report
class SomaticReportHelper
{
public:
	///Constructor loads data into class
	SomaticReportHelper(QString snv_filename, const ClinCnvList& filtered_cnvs, const FilterCascade& filters, const QString& target_region="");
	///write Rtf File
	void writeRtf(const QByteArray& out_file);

	///methods that create files for QBIC
	void somaticSnvForQbic();
	void germlineSnvForQbic();
	void somaticCnvForQbic();
	void germlineCnvForQbic();
	void somaticSvForQbic();
	void metaDataForQbic();

	///returns CGI cancertype if available from VariantList
	static QByteArray cgiCancerTypeFromVariantList(const VariantList& variants);

private:
	///transforms GSVar coordinates of Variants to VCF INDEL-standard
	VariantList gsvarToVcf(const VariantList& gsvar_list, const QString& orig_name);

	///returns best matching transcript - or an empty transcript
	VariantTranscript selectSomaticTranscript(const Variant& variant);

	RtfTable createGapStatisticsTable(const QList<int>& col_widths);
	QHash<QByteArray, BedFile> gapStatistics(const BedFile& region_of_interest);

	///Writes Rtf table containing given snvs
	RtfTable createSnvTable(const QList<int>& col_widths, const VariantList& snvs);
	///Writes Rtf table containing CNVs per gene
	RtfTable createCNVdriverTable(const GeneSet& target_genes);
	///generates table with CNVs
	RtfTable createCnvTable();

	///Writes table with drug annotation
	RtfTable createCgiDrugTable();

	///Writers basic QC params to RTF report
	RtfTable createQCTable(const QList<int>& widths);

	///Parse raw text containing CGI cancer acronyms in the form "known in:
	QList<QByteArray> parse_cgi_cancer_acronyms(QByteArray text);

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

	///List containing cgi acronyms, full text (english and german) and tumor mutation burden for several cancer types (source: doi:10.1186/s13073-017-0424-2)
	QList<cgi_info> cgi_dictionary_;

	///ICD10 text diagnosis tumor
	QString icd10_diagnosis_code_;
	///tumor fraction according genlab
	QString histol_tumor_fraction_;

	///HPO term listed in NGSD
	QString hpo_term_;

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
	int cnv_index_tumor_clonality_;
	int cnv_index_tumor_cn_change_;

	///List of CGI cancer abbreviations that occur ANYWHERE in the report
	QByteArrayList cgi_acronyms_;

	///Filter list
	FilterCascade filters_;

	RtfDocument doc_;
};

#endif // SomaticReportHelper_H
