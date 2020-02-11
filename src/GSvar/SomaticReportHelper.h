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
#include <QDialog>
#include <QMultiMap>
#include <QDir>
#include "RtfDocument.h"
#include "BedpeFile.h"
#include "SomaticReportSettings.h"

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

	bool showInReport()
	{
		return show_in_report_;
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

	void setShowInReport(bool show)
	{
		show_in_report_ = show;
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

	///This DrugReportLine will appear in report
	bool show_in_report_ = true;
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


struct somatic_virus
{
	QByteArray chr_;
	int start_;
	int end_;
	QByteArray name_;
	int reads_;
	double coverage_;
	int mismatches_;
	double idendity_;

	///Virus gene, extracted from name_ if possible
	QByteArray virusGene() const
	{
		QByteArray gene_name = name_;
		if(name_.split('_').count() > 1)
		{
			QByteArray reduced_name = name_.split('_').at(0) + "_";
			gene_name = gene_name.replace(reduced_name,"");
			return gene_name;
		}
		return "";
	}

	QByteArray virusName() const
	{
		QByteArray virus_name = "";
		if(name_.split('_').count() > 0)
		{
			virus_name = name_.split('_').at(0);
		}
		return virus_name;
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
	SomaticReportHelper(const VariantList& variants, const CnvList &cnvs, const VariantList& germline_variants, const SomaticReportSettings& settings);

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

	///Parses CGI driver statement into German language
	static QByteArray CgiDriverDescription(QByteArray raw_cgi_input);

	///Returns CNV size description, e.g. "fokal" or "Cluster", in German language
	static QByteArray CnvSizeDescription(QByteArray orig_entry);

	///Returns CNV type, e.g. DEL (het) according copy number
	static QByteArray CnvTypeDescription(int tumor_cn);

	///Returns true if all required annotation (CGI, NCG) are available
	static bool checkRequiredSNVAnnotations(const VariantList& snvs);

	///Returns maximum tumor clonailty in cnv file
	static double getCnvMaxTumorClonality(const CnvList& cnvs);

	///Returns CNV burden, i.e. total cnv size divided by genome size in %
	static double cnvBurden(const CnvList& cnvs)
	{
		return cnvs.totalCnvSize() / 3101788170. * 100;
	}

private:
	///transforms GSVar coordinates of Variants to VCF INDEL-standard
	VariantList gsvarToVcf(const VariantList& gsvar_list, const QString& orig_name);

	///returns best matching transcript - or an empty transcript
	VariantTranscript selectSomaticTranscript(const Variant& variant);

	RtfTable createGapStatisticsTable(const QList<int>& col_widths);
	QMap<QByteArray, BedFile> gapStatistics(const BedFile& region_of_interest);

	///Writes Rtf table containing most relevant SNVs and CNVs
	RtfTable somaticAlterationTable(const VariantList& snvs, const CnvList& cnvs, bool include_cnvs, const GeneSet& target_genes = GeneSet());

	///generates table with CNVs
	RtfTable createCnvTable();

	///Writes table with drug annotation
	RtfTable createCgiDrugTable();

	///Creates table containing alterations relevant in pharmacogenomics (from normal sample)
	RtfTable pharamacogeneticsTable();

	RtfTable germlineAlterationTable(const VariantList& somatic_snvs);

	///Returns text for fusions, appears multiple times in report
	RtfParagraph fusionsText()
	{
		if(fusions_.count() > 0)
		{
			return RtfParagraph("Fusionen gefunden. Bitte Datei mit Strukturvarianten prüfen.").highlight(3);
		}
		else
		{
			return RtfParagraph("Es wurde keine der untersuchten Fusionen nachgewiesen.");
		}
	}

	QString trans(const QString& text) const;


	RtfTableRow tumorContent();

	const SomaticReportSettings& settings_;

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
	QString tumor_ps_;

	///normal ID
	QString normal_ps_;

	///Input VariantList
	VariantList snv_variants_;

	///VariantList for relevant germline SNVs
	const VariantList& snv_germline_;

	///CNVList for input (filtered) variants
	CnvList cnvs_;

	///Somatic viruses (original file usually in tumor dir)
	QList<somatic_virus> validated_viruses_;

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
	double histol_tumor_fraction_;

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
	int cnv_index_cn_change_;
	int cnv_index_cgi_gene_role_;
	int cnv_index_cnv_type_;
	int cnv_index_cgi_genes_;
	int cnv_index_cgi_driver_statement_;
	int cnv_index_tumor_clonality_;
	int cnv_index_tumor_cn_change_;

	///List of CGI cancer abbreviations that occur ANYWHERE in the report
	QByteArrayList cgi_acronyms_;

	RtfDocument doc_;

	BedpeFile fusions_;

};

#endif // SomaticReportHelper_H
