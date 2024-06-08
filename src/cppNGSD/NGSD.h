#ifndef NGSD_H
#define NGSD_H

#include "cppNGSD_global.h"
#include <QVariant>
#include <QVariantList>
#include <QSharedPointer>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include "VariantList.h"
#include "BedFile.h"
#include "Transcript.h"
#include "QCCollection.h"
#include "SqlQuery.h"
#include "GeneSet.h"
#include "PhenotypeList.h"
#include "Helper.h"
#include "DBTable.h"
#include "ReportConfiguration.h"
#include "SomaticReportConfiguration.h"
#include "CnvList.h"
#include "BedpeFile.h"
#include "SomaticVariantInterpreter.h"
#include "SomaticCnvInterpreter.h"
#include "NGSHelper.h"
#include "FileLocation.h"
#include "FileInfo.h"
#include "TsvFile.h"
#include "HttpRequestHandler.h"

///Sample relation datastructure
struct CPPNGSDSHARED_EXPORT SampleRelation
{
	QByteArray sample1; //sample name, not identifier
	QByteArray relation;
	QByteArray sample2; //sample name, not identifier
};

///OMIM information datastructure
struct CPPNGSDSHARED_EXPORT OmimInfo
{
	QByteArray gene_symbol;
	QByteArray mim;

	PhenotypeList phenotypes;
};

///KASP data
struct CPPNGSDSHARED_EXPORT KaspData
{
	int snps_evaluated;
	int snps_match;
	double random_error_prob;
};

///Type constraints class for database fields
struct CPPNGSDSHARED_EXPORT TableFieldConstraints
{
	QStringList valid_strings; //ENUM (from schema)
	int max_length; //VARCHAR (from schema)
	QRegularExpression regexp; //VARCHAR
};

///General database field information.
struct CPPNGSDSHARED_EXPORT TableFieldInfo
{
	enum Type
	{
		BOOL,
		INT,
		FLOAT,
		TEXT, //multi-line text
		VARCHAR, //one line text
		VARCHAR_PASSWORD, //note: special handling when shown and edited
		ENUM,
		SET,
		DATE,
		DATETIME, //note: hidden by default
		TIMESTAMP, //note: hidden by default
		FK //foreign-key
	};

	int index = -1;
	QString name;

	//type info
	Type type;
	bool is_nullable = false;
	bool is_unsigned = false;
	TableFieldConstraints type_constraints;
	QString default_value;

	//index+key info
	bool is_primary_key = false;
	bool is_unique = false;
	QString fk_table; //target table of FK
	QString fk_field; //target field of FK
	QString fk_name_sql; //SQL code to get the name in the target table - normally 'name', but can contain any valid SQL query

	//displaying options
	QString label; //label to show (normally the name, but overwritten e.g. for FK fields)
	bool is_hidden = false; //not shown
	bool is_readonly = false; //shown, but not editable (after it is initially set)
	QString tooltip; //tooltip taken from column comment of the SQL database

	///Returns type as string.
	QString typeAsString() const
	{
		return typeToString(type);
	}
	///String representation of the table field
	QString toString() const;

	///Converts type to human-readable string.
	static QString typeToString(Type type);

	///Returns if the given text is a valid entry for this field.
	bool isValid(QString text) const;
};

///General database table information.
class CPPNGSDSHARED_EXPORT TableInfo
{

	public:
		const QString& table() const
		{
			return table_;
		}
		void setTable(const QString& table)
		{
			table_ = table;
		}

		const QList<TableFieldInfo>& fieldInfo() const
		{
			return field_infos_;
		}
		void setFieldInfo(const QList<TableFieldInfo>& fields)
		{
			field_infos_ = fields;
		}

		///Returns information about a specific field.
		const TableFieldInfo& fieldInfo(const QString& field_name) const
		{
			foreach(const TableFieldInfo& entry, field_infos_)
			{
				if (entry.name==field_name)
				{
					return entry;
				}
			}

			THROW(DatabaseException, "Field '" + field_name + "' not found in NGSD table '" + table_ + "'!");
		}

		///Returns the all field names.
		QStringList fieldNames() const
		{
			QStringList output;

			foreach(const TableFieldInfo& entry, field_infos_)
			{
				output << entry.name;
			}

			return output;
		}

		bool fieldExists(const QString& field_name) const
		{
			foreach(const TableFieldInfo& entry, field_infos_)
			{
				if (entry.name==field_name)
				{
					return true;
				}
			}

			return false;
		}

		int fieldCount() const
		{
			return field_infos_.count();
		}

	protected:
		QString table_;
		QList<TableFieldInfo> field_infos_;
};

///Analysis job sample.
struct CPPNGSDSHARED_EXPORT AnalysisJobSample
{
	QString name;
	QString info;

	bool operator==(const AnalysisJobSample& rhs) const
	{
		return name==rhs.name && info==rhs.info;
	}
};

///Analysis job sample.
struct CPPNGSDSHARED_EXPORT AnalysisJobHistoryEntry
{
	QDateTime time;
	QString user;
	QString status;
	QStringList output;

	QString timeAsString() const
	{
		return time.toString(Qt::ISODate).replace('T', ' ');
	}
};

///Analysis job information.
struct CPPNGSDSHARED_EXPORT AnalysisJob
{
	QString type;
	bool high_priority;
	QString args;
	QString sge_id;
	QString sge_queue;

	QList<AnalysisJobSample> samples;
	QList<AnalysisJobHistoryEntry> history;

	//Returns the last status
	QString finalStatus()
	{
		return history.count()==0 ? "n/a" : history.last().status;
	}

	//Returns if the job is still running
	bool isRunning()
	{
		return finalStatus()=="queued" || finalStatus()=="started";
	}

	//Returns the run time in human-readable format (so far if still running)
	QString runTimeAsString() const;
};

///Sample group information
struct CPPNGSDSHARED_EXPORT SampleGroup
{
	QString name;
	QString comment;
};

///Sample information.
struct CPPNGSDSHARED_EXPORT SampleData
{
	QString name;
	QString name_external;
	QString patient_identifier;
	QString year_of_birth;
	QString type;
	QString gender;
	QString quality;
	QString comments;
	QString disease_group;
	QString disease_status;
	QString tissue;
	PhenotypeList phenotypes;
	bool is_tumor;
	bool is_ffpe;
	QString sender;
	QString species;
	QString received;
	QString received_by;
	QString order_date;
	QString sampling_date;
	QList<SampleGroup> sample_groups;
};

///Sample disease information.
struct CPPNGSDSHARED_EXPORT SampleDiseaseInfo
{
	QString disease_info;
	QString type;
	QString user;
	QDateTime date;
};

///Processed sample information.
struct CPPNGSDSHARED_EXPORT ProcessedSampleData
{
	QString name;
	QString processing_system;
	QString processing_system_type;
	QString sequencer_type;
	QString quality;
	QString gender;
	QString comments;
	QString project_name;
	QString project_type;
	QString run_name;
	QString normal_sample_name;
	QString lab_operator;
	QString processing_modus;
	QString batch_number;
	QString processing_input;
	QString molarity;
	QString ancestry;
	bool scheduled_for_resequencing;
};

///Processing system information.
struct CPPNGSDSHARED_EXPORT ProcessingSystemData
{
	QString name;
	QString name_short;
	QString adapter1_p5;
	QString adapter2_p7;
	bool shotgun;
	QString umi_type;
	QString type;
	QString genome;
};

/// Germline gene information.
struct CPPNGSDSHARED_EXPORT GeneInfo
{
	//gene symbol
	QString symbol;
	//notice about the symbol based on HGNC data (unknown symbol, previous symbol, etc.)
	QString symbol_notice;
	//gene name
	QString name;
	//HGNC identifier
	QString hgnc_id;
	//HGNC locus group
	QString locus_group;

	//gene inheritance mode
	QString inheritance;

	//gnomAD o/e score for synonymous variants (default is NULL).
	QString oe_syn;
	//gnomAD o/e score for missense variants (default is NULL).
	QString oe_mis;
	//gnomAD o/e score for loss-of-function variants (default is NULL).
	QString oe_lof;

	//status of imprinting information
	QString imprinting_status;
	//sources allele of imprinted gene
	QString imprinting_source_allele;

	//list of pseudogenes (not all are HGNC-approved symbols)
	QStringList pseudogenes;

	//comments
	QString comments;

	//returns the main gene information as a string
	QString toString()
	{
		return symbol + " (inh=" + inheritance + " oe_syn=" + oe_syn + " oe_mis=" + oe_mis + " oe_lof=" + oe_lof + ")";
	}
};

///Variant classification information
struct CPPNGSDSHARED_EXPORT ClassificationInfo
{
	QString classification;
	QString comments;
};

///Diagnostic status and report outcome information
struct CPPNGSDSHARED_EXPORT DiagnosticStatusData
{
	//main data
	QString dagnostic_status;
	QString outcome = "n/a";
	QString comments = "";

	//meta data
	QString user;
	QDateTime date;
};

///Search parameters for processed samples
struct CPPNGSDSHARED_EXPORT ProcessedSampleSearchParameters
{
	//filters sample
	QString s_name;
	bool s_name_ext = false;
	bool s_name_comments = false;
	QString s_patient_identifier;
	QString s_species;
	QString s_type;
	QString s_sender;
	QString s_study;
	QString s_disease_group;
	QString s_disease_status;
	PhenotypeList s_phenotypes;
	QString s_tissue;
	QString s_ancestry;
	bool include_bad_quality_samples = true;
	bool include_tumor_samples = true;
	bool include_germline_samples = true;
	bool include_ffpe_samples = true;
	bool include_merged_samples = false;
	bool include_scheduled_for_resequencing_samples = false;
	bool only_with_small_variants = false;

	//filters project
	QString p_name;
	QString p_type;
	bool include_archived_projects = true;

	//filters processing system
	QString sys_name;
	QString sys_type;

	//filters sequencing run
	QString r_name;
	bool include_bad_quality_runs = true;
	bool run_finished = false;
	QDate r_before = QDate();
	QDate r_after = QDate();
	QString r_device_name;

	//filter output to processed samples that user has access to
	QString restricted_user;

	//output options
	QString add_path;
	bool add_disease_details = false;
	bool add_outcome = false;
	bool add_qc = false;
	bool add_report_config = false;
	bool add_comments = false;
	bool add_normal_sample = false;
	bool add_dates = false;
	bool add_call_details = false;
	bool add_lab_columns = false;
};

///Meta data about somatic report configuration (e.g. creation/update, target bed file)
struct CPPNGSDSHARED_EXPORT SomaticReportConfigurationData
{
	QString created_by;
	QString created_date;
	QString last_edit_by;
	QString last_edit_date;

	QString target_file;
	QString mtb_xml_upload_date;

	///Returns a text representation of the creation and update. Can contain newline!
	QString history() const;
};

///Header data of the Evaluation Sheet
struct CPPNGSDSHARED_EXPORT EvaluationSheetData
{
	//set default values on construction
	EvaluationSheetData() :
		build(GenomeBuild::HG38),
		acmg_requested(false),
		acmg_noticeable(false),
		acmg_analyzed(false),
		filtered_by_freq_based_dominant(false),
		filtered_by_freq_based_recessive(false),
		filtered_by_mito(false),
		filtered_by_x_chr(false),
		filtered_by_cnv(false),
		filtered_by_svs(false),
		filtered_by_res(false),
		filtered_by_mosaic(false),
		filtered_by_phenotype(false),
		filtered_by_multisample(false),
		filtered_by_trio_stringent(false),
		filtered_by_trio_relaxed(false)
	{}

	GenomeBuild build;
	QString ps_id;
	QString dna_rna;
	QString reviewer1;
	QDate review_date1;
	QString reviewer2;
	QDate review_date2;

	QString analysis_scope;
	bool acmg_requested;
	bool acmg_noticeable;
	bool acmg_analyzed;

	bool filtered_by_freq_based_dominant;
	bool filtered_by_freq_based_recessive;
	bool filtered_by_mito;
	bool filtered_by_x_chr;
	bool filtered_by_cnv;
	bool filtered_by_svs;
	bool filtered_by_res;
	bool filtered_by_mosaic;
	bool filtered_by_phenotype;
	bool filtered_by_multisample;
	bool filtered_by_trio_stringent;
	bool filtered_by_trio_relaxed;
};

/// Metadata of cfDNA panel DB entry
struct CfdnaPanelInfo
{
	int id = -1;
	int tumor_id = -1;
	int cfdna_id = -1;
	int created_by = -1;
	QDate created_date;
	int processing_system_id = -1;
};

/// cfDNA Gene entry
struct CfdnaGeneEntry
{
	QString gene_name;
	Chromosome chr;
	int start;
	int end;
	QDate date;
	BedFile bed = BedFile();
};

///NGSD import status for germline analysis.
struct CPPNGSDSHARED_EXPORT ImportStatusGermline
{
	//variant data
	int small_variants = 0;
	int cnvs = 0;
	int svs = 0;
	//QC data
	int qc_terms = 0;
};

/// statistics data on RNA expression
struct ExpressionStats
{
	double mean;
	double mean_log2;
	double stddev_log2;
};

/// Keeps track of sample information for multi-sample analyses
struct CPPNGSDSHARED_EXPORT MultiSampleAnalysisInfo
{
	QString analysis_file;
	QString analysis_name;
	QStringList ps_sample_name_list;
	QStringList ps_sample_id_list;
};

///NGSD RNA cohort determination stategy
enum RnaCohortDeterminationStategy
{
	RNA_COHORT_GERMLINE, //based on processing system
	RNA_COHORT_GERMLINE_PROJECT, //based on processing system and project
	RNA_COHORT_SOMATIC, //based on HPO or ICD10
	RNA_COHORT_CUSTOM //list of processed samples needs to be provided
};

///Same sample relation mode
enum SameSampleMode
{
	SAME_SAMPLE, //only consider samples from the same biological sample
	SAME_PATIENT //consider samples from the same sample, patient or same patient id
};

///Custom structs for data exchange

///cfDNA disease course table
struct CfdnaDiseaseCourseTable
{
	struct CfdnaDiseaseCourseTableCfdnaEntry
	{
		double multi_af;
		int multi_alt;
		int multi_ref;
		double p_value;
	};
	struct CfdnaDiseaseCourseTableLine
	{
		VcfLine tumor_vcf_line;
		QList<CfdnaDiseaseCourseTableCfdnaEntry> cfdna_columns;
	};
	struct PSInfo
	{
		QString name;
		QString ps_id;
		QDate received_date;
		QDate sampling_date;
		QDate order_date;

		bool operator<(const PSInfo& other) const {
			if (sampling_date.isValid() && other.sampling_date.isValid())
			{
				return sampling_date < other.sampling_date;
			}
			else
			{
				return received_date < other.received_date; // sort by date
			}
		}
	};

	//sample info
	PSInfo tumor_sample;
	QList<PSInfo> cfdna_samples;

	//table content
	QList<CfdnaDiseaseCourseTableLine> lines;

	//mrd values
	QList<TsvFile> mrd_tables;
};

///custum struct to store the submission status of ClinVarUploads
struct CPPNGSDSHARED_EXPORT ClinvarSubmissionStatus
{
	QString status;
	QString stable_id;
	QString comment;
};

///Variant genotype counts
struct CPPNGSDSHARED_EXPORT GenotypeCounts
{
	int hom;
	int het;
	int mosaic;
};

///Variant calling details
struct CPPNGSDSHARED_EXPORT VariantCallingInfo
{
	QString small_caller;
	QString small_caller_version;
	QString small_call_date; //ISO format

	QString cnv_caller;
	QString cnv_caller_version;
	QString cnv_call_date; //ISO format

	QString sv_caller;
	QString sv_caller_version;
	QString sv_call_date; //ISO format
};

///NGSD access
class CPPNGSDSHARED_EXPORT NGSD
		: public QObject
{
Q_OBJECT

public:
	///Default constructor that connects to the DB
	NGSD(bool test_db=false, QString name_suffix="");
	///Destructor.
	~NGSD();
	///Returns if the database connection is (still) open
	bool isOpen() const;
	///Returns if the database is a production database based on information in the table 'db_info'.
	bool isProductionDb() const;

	///Returns if the database is available (i.e. the credentials are in the settings file or the application is in client-server mode)
	static bool isAvailable(bool test_db=false);

	///Returns the table list.
	QStringList tables() const;
	///Returns information about all fields of a table.
	const TableInfo& tableInfo(const QString& table, bool use_cache = true) const;
	///Checks if the value is valid for the table/field when used in an SQL query. Returns a non-empty error list in case it is not. 'check_unique' must not be used for existing entries.
	QStringList checkValue(const QString& table, const QString& field, const QString& value, bool check_unique) const;
	///Escapes SQL special characters in a text
	QString escapeText(QString text);

	///Creates a SQL dump for a given table. sql_history is a hash table that keeps track of already exported records: table name > exported IDs set.
    void exportTable(const QString& table, QTextStream& out, QString where_clause = "", QMap<QString, QSet<int>> *sql_history = nullptr);

	///Creates a DBTable with data from an SQL query.
	DBTable createTable(QString table, QString query, int pk_col_index=0);
	///Creates a DBTable with all rows of a table.
	DBTable createOverviewTable(QString table, QString text_filter = QString(), QString sql_order="id DESC", int pk_col_index=0);
	///Replace table column with foreign key IDs by names
	void replaceForeignKeyColumn(DBTable& table, int column, QString fk_table, QString fk_name_sql);

	///Creates database tables and imports initial data (password is required for production database if it is not empty)
	void init(QString password="");

	/*** General database functionality ***/
	///Executes an SQL query and returns the single return value.
	///If no values are returned an error thrown or a default-constructed QVariant is returned (depending on @p empty_is_ok).
	///If more than one value is returned a DatabaseError is thrown.
	///If @p bind_value is set, the placeholder ':0' in the query is replaced with it (SQL special characters are replaced).
	QVariant getValue(const QString& query, bool no_value_is_ok=true, QString bind_value = QString()) const;
	///Executes an SQL query and returns the text value list.
	///If @p bind_value is set, the placeholder ':0' in the query is replaced with it (SQL special characters are replaced).
	QStringList getValues(const QString& query, QString bind_value = QString()) const;
	///Executes an SQL query and returns the integer value list.
	///If @p bind_value is set, the placeholder ':0' in the query is replaced with it (SQL special characters are replaced).
	QList<int> getValuesInt(const QString& query, QString bind_value = QString()) const;
	///If @p bind_value is set, the placeholder ':0' in the query is replaced with it (SQL special characters are replaced).
	QVector<double> getValuesDouble(const QString& query, QString bind_value = QString()) const;
	///Returns a SqlQuery object on the NGSD for custom queries.
	SqlQuery getQuery() const
	{
		return SqlQuery(*db_);
	}
	///Executes all queries from a text file.
	void executeQueriesFromFile(QString filename);

	///Returns all possible values for a enum column.
	QStringList getEnum(QString table, QString column) const;
	///Checks if a table exists.
	bool tableExists(QString table, bool throw_error_if_not_existing=true) const;
	///Checks if a row the given id exists in the table.
	bool rowExists(QString table, int id) const;
	///Checks if a table is empty.
	bool tableEmpty(QString table) const;
	///Clears all contents from a table.
	void clearTable(QString table);


	/*** transactions ***/
	bool transaction();
	bool commit();
	bool rollback();

	/*** gene/transcript handling ***/
	///Returns the gene ID, or -1 if no approved gene name could be found. Checks approved symbols, previous symbols and synonyms. Uses internal cache to speed up repeated queries of the same gene name.
	int geneId(const QByteArray& gene);
	///Returns the gene ID of the transcript, or -1 if no gene could be determined.
	int geneIdOfTranscript(const QByteArray& name, bool throw_on_error=true, GenomeBuild build=GenomeBuild::HG38);
	///Returns the gene symbol for a gene ID.
	QByteArray geneSymbol(int id);
	///Returns the HGNC identifier of a gene.
	QByteArray geneHgncId(int id);
	///Returns the approved gene symbol or "" if it could not be determined.
	QByteArray geneToApproved(QByteArray gene, bool return_input_when_unconvertable=false);
	///Returns the approved gene symbols.
	GeneSet genesToApproved(GeneSet genes, bool return_input_when_unconvertable=false);
	///Returns the approved/original gene symbol and a status message.
	QPair<QString, QString> geneToApprovedWithMessage(const QString& gene);
	///Returns the approved/original gene symbol and a status message - if ambugous several pairs are returned.
	QList<QPair<QByteArray, QByteArray>> geneToApprovedWithMessageAndAmbiguous(const QByteArray& gene);
	///Returns previous symbols of a gene.
	GeneSet previousSymbols(int id);
	///Returns aliases of a gene.
	GeneSet synonymousSymbols(int id);
	///Returns the genes overlapping the given regions (extended by some bases)
	GeneSet genesOverlapping(const Chromosome& chr, int start, int end, int extend=0);
	///Returns the genes overlapping the given region (extended by some bases)
	GeneSet genesOverlappingByExon(const Chromosome& chr, int start, int end, int extend=0);
	///Returns the chromosomal regions corresponding to the given gene. Messages about unknown gene symbols etc. are written to the steam, if given.
	BedFile geneToRegions(const QByteArray& gene, Transcript::SOURCE source, QString mode, bool fallback = false, bool annotate_transcript_names = false, QTextStream* messages = nullptr);
	///Returns the chromosomal regions corresponding to the given genes. Messages about unknown gene symbols etc. are written to the steam, if given.
	BedFile genesToRegions(const GeneSet& genes, Transcript::SOURCE source, QString mode, bool fallback = false, bool annotate_transcript_names = false, QTextStream* messages = nullptr);
	///Returns the chromosomal regions corresponding to the given transcript. Messages about unknown transcripts etc. are written to the steam, if given.
	BedFile transcriptToRegions(const QByteArray& name, QString mode);
	///Returns transcript by id. Throws an exception if not found in NGSD.
	const Transcript& transcript(int id);
	///Returns transcript identifier. Throws an exception if not found in NGSD, or returns -1.
	int transcriptId(QString name, bool throw_on_error=true);
	///Returns all transcripts in the database;
	const TranscriptList& transcripts();
	///Returns transcripts of a gene (if @p coding_only is set, only coding transcripts).
	TranscriptList transcripts(int gene_id, Transcript::SOURCE source, bool coding_only);
	///Returns all transcripts overlapping the given region (extended by some bases).
	TranscriptList transcriptsOverlapping(const Chromosome& chr, int start, int end, int extend=0, Transcript::SOURCE source=Transcript::ENSEMBL);
	///Returns the best transcript for the gene. Order is: (longest coding) preferred transcript, MANE select transcript, ensemble canonical, longest coding transcript, longest non-coding transcript, longest transcript. If no transcript is found, a invalid default-constructed transcript is returned.
	/// The return_quality int is higher the higher the quality of the returned transcript is. Exact numbers may not be constant: preferred > MANE > canonical > longest coding , longest non-coding , not found
	Transcript bestTranscript(int gene_id, const QList<VariantTranscript> var_transcripts=QList<VariantTranscript>(), int *return_quality=nullptr);
	//Return the transcript with the highest impact given the variant transcript impacts
	Transcript highestImpactTranscript(TranscriptList transcripts, const QList<VariantTranscript> var_transcripts);
	///Returns a list of the most relevant transcripts for the gene (best transcript, prefered transcripts, MANE select transcript, MANE plus clinical transcript)
	TranscriptList releventTranscripts(int gene_id);
	///Returns longest coding transcript of a gene.
	Transcript longestCodingTranscript(int gene_id, Transcript::SOURCE source, bool fallback_alt_source=false, bool fallback_noncoding=false);
	///Returns the list of all approved gene names
	const GeneSet& approvedGeneNames();
	///Returns the map of gene to preferred transcripts (note: transcript names do not contain version numbers)
	QMap<QByteArray, QByteArrayList> getPreferredTranscripts();
	///Adds a preferred transcript. Returns if it was added, i.e. it was not already present. Throws an exception, if the transcript name is not valid.
	bool addPreferredTranscript(QByteArray transcript_name);

	/*** phenotype handling (HPO, OMIM) ***/
	///Returns the NGSD database ID of the phenotype given. Returns -1 or throws a DatabaseException if term name does not exist.
	int phenotypeIdByAccession(const QByteArray& accession, bool throw_on_error=true);
	///Returns the NGSD database ID of the phenotype given. Returns -1 or throws a DatabaseException if term name does not exist. Prefer phenotypeIdByAccession whenever possible since it is faster!
	int phenotypeIdByName(const QByteArray& name, bool throw_on_error=true);
	///Returns the phenotype for a given HPO accession.
	const Phenotype& phenotype(int id);
	///Returns the phenotypes of a gene
	PhenotypeList phenotypes(const QByteArray& symbol);
	///Returns all phenotypes matching the given search terms (or all terms if no search term is given)
	PhenotypeList phenotypes(QStringList search_terms);
	///Returns all genes associated to a phenotype. If 'ignore_non_phenotype_terms' is set, only children of 'Phenotypic abnormality' are returned.
	GeneSet phenotypeToGenes(int id, bool recursive, bool ignore_non_phenotype_terms=true);
	///Returns all genes associated with a phenotype that fullfil the allowed source/evidence criteria. If 'ignore_non_phenotype_terms' is set, only children of 'Phenotypic abnormality' are returned.
	GeneSet phenotypeToGenesbySourceAndEvidence(int id, QSet<PhenotypeSource> allowed_sources, QSet<PhenotypeEvidenceLevel> allowed_evidences, bool recursive, bool ignore_non_phenotype_terms=true);
	///Returns all child terms of the given phenotype
	PhenotypeList phenotypeChildTerms(int term_id, bool recursive);
	///Returns all parent terms of the given phenotype
	PhenotypeList phenotypeParentTerms(int term_id, bool recursive);
	///Returns OMIM information for a gene. Several OMIM entries per gene are rare, but happen e.g. in the PAR region.
	QList<OmimInfo> omimInfo(const QByteArray& symbol);
	///Returns the accession (6 digit number) of the preferred OMIM phenotype for a gene. If unset, an empty string is returned.
	QString omimPreferredPhenotype(const QByteArray& symbol, const QByteArray& disease_group);

	/*** Base functionality for file/variant processing ***/
	///Returns the sample name for an ID. If it does not exist, an exception is thrown or an empty string is returned.
	QString sampleName(const QString& s_id, bool throw_if_fails = true);
	///Returns the processed sample name for an ID. If it does not exist, an exception is thrown or an empty string is returned.
	QString processedSampleName(const QString& ps_id, bool throw_if_fails = true);
	///Returns the NGSD sample ID file name. Throws an exception if it could not be determined.
	QString sampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD processed sample ID from a file name or processed sample name. Throws an exception if it could not be determined.
	QString processedSampleId(const QString& filename, bool throw_if_fails = true);	
	///Removes init data for the database
	void removeInitData();

	///Returns the project folder for a project type
	QString projectFolder(QString type);
	///Returns the path of certain file of a processed sample (type)
	QString processedSamplePath(const QString& processed_sample_id, PathType type);
	///Returns the path to secondary analyses of the processed samples.
	QStringList secondaryAnalyses(QString processed_sample_name, QString analysis_type);

	///Adds a variant to the NGSD. Returns the variant ID.
	QString addVariant(const Variant& variant);
	///Adds a variant to the NGSD including gnomAD AF and coding/splicing information. Returns the variant ID.
	QString addVariant(const Variant& variant, const VariantList& variant_list);
	///Adds all missing variants to the NGSD and returns the variant DB identifiers (or -1 if the variant was skipped due to 'max_af' or because it is over 500 bases long)
	QList<int> addVariants(const VariantList& variant_list, double max_af, int& c_add, int& c_update);
	///Returns the NGSD ID for a variant. Returns '' or throws an exception if the ID cannot be determined.
	QString variantId(const Variant& variant, bool throw_if_fails = true);
	///Returns the variant corresponding to the given identifier or throws an exception if the ID does not exist.
	Variant variant(const QString& variant_id);
	///Returns the number of hom/het/mosaic occurances of the variant in the NGSD (only one occurance per sample is counted).
	GenotypeCounts genotypeCounts(const QString& variant_id);
	///Returns the number of hom/het/mosaic occurances of the variant in the NGSD (only one occurance per sample is counted) - returns counts cached in 'variant' table which is faster.
	GenotypeCounts genotypeCountsCached(const QString& variant_id);
	///Deletes the variants of a processed sample (all types)
	void deleteVariants(const QString& ps_id);
	///Deletes the variants of a processed sample (a specific type)
	void deleteVariants(const QString& ps_id, VariantType type);

	///Returns the repeat expansion NGSD ID
	QString repeatExpansionId(const QString& region, const QString& repeat_unit, bool throw_if_fails=true);
	///Returns the (rich text) comments of the repeat with the given ID
	QString repeatExpansionComments(int id);

	///Adds PubMed ID to a variant
	void addPubmedId(int variant_id, const QString& pubmed_id);
	///Returns all PubMed IDs for a given variant id
	QStringList pubmedIds(const QString& variant_id);

	void deleteSomaticVariants(QString t_ps_id, QString n_ps_id);
	void deleteSomaticVariants(QString t_ps_id, QString n_ps_id, VariantType type);

	///Adds a CNV to the NGSD. Returns the CNV ID.
	QString addCnv(int callset_id, const CopyNumberVariant& cnv, const CnvList& cnv_list, double max_ll = 0.0);
	///Returns the NGSD ID for a CNV. Returns '' or throws an exception if the ID cannot be determined.
	QString cnvId(const CopyNumberVariant& cnv, int callset_id, bool throw_if_fails = true);
	///Returns the CNV corresponding to the given identifiers or throws an exception if the ID does not exist.
	CopyNumberVariant cnv(int cnv_id);

	///Adds a SV to the NGSD. Returns the SV ID.
	int addSv(int callset_id, const BedpeLine& structuralVariant, const BedpeFile& svs);
	///Returns the NGSD ID for a SV. Returns '' or throws an exception if the ID cannot be determined.
	QString svId(const BedpeLine& sv, int callset_id, const BedpeFile& svs, bool throw_if_fails = true);
	///Returns the SV corresponding to the given identifiers or throws an exception if the ID does not exist.
	///		'no_annotation' will only return the SV position
	BedpeLine structuralVariant(int sv_id, StructuralVariantType type, const BedpeFile& svs, bool no_annotation = false, int* callset_id = 0);
	///Returns the SQL table name for a given StructuralVariantType
	static QString svTableName(StructuralVariantType type);


	///Adds a somatic CNV to the NGSD. Returns the somatic CNV ID.
	QString addSomaticCnv(int callset_id, const CopyNumberVariant& cnv, const CnvList& cnv_list, double max_ll = 0.0);
	QString somaticCnvId(const CopyNumberVariant& cnv, int callset_id, bool throw_if_fails = true);
	CopyNumberVariant somaticCnv(int cnv_id);

	///Returns the germline import status.
	ImportStatusGermline importStatus(const QString& ps_id);

	/* RNA related functions */
	///Imports gene expression data to the NGSD
	void importGeneExpressionData(const QString& expression_data_file_path, const QString& ps_name, bool force, bool debug);
	int addGeneSymbolToExpressionTable(const QByteArray& gene_symbol);
	///Imports exon expression data to the NGSD
	void importExonExpressionData(const QString& expression_data_file_path, const QString& ps_name, bool force, bool debug);
	///Calculates statistics on all gene expression values for a list of processed sample ids
	QMap<QByteArray, ExpressionStats> calculateGeneExpressionStatistics(QSet<int>& cohort, QByteArray gene="", bool debug=false);
	///Calculates statistics on all exon expression values for a list of processed sample ids
	QMap<QByteArray, ExpressionStats> calculateExonExpressionStatistics(QSet<int>& cohort, const BedLine& exon=BedLine(), bool debug=false);
	///Calculates statistics on all expression values of the same processing system and tissue
	QMap<QByteArray, ExpressionStats> calculateCohortExpressionStatistics(int sys_id, const QString& tissue_type, QSet<int>& cohort, const QString& project="", const QString& ps_id="",
																	RnaCohortDeterminationStategy cohort_type=RNA_COHORT_GERMLINE, const QStringList& exclude_quality=QStringList() << "bad", bool debug=false);
	///Determines the sample cohort for a given sample
	QSet<int> getRNACohort(int sys_id, const QString& tissue_type, const QString& project="", const QString& ps_id="", RnaCohortDeterminationStategy cohort_type=RNA_COHORT_GERMLINE,
						   const QByteArray& mode = "genes", const QStringList& exclude_quality=QStringList() << "bad", bool debug=false);
	///Creates a mapping from ENSG ensembl identifier to NGSD gene ids
	QMap<QByteArray, QByteArray> getEnsemblGeneMapping();
	///Creates a mapping from gene symbols to ENSG ensembl identifier
	QMap<QByteArray, QByteArray> getGeneEnsemblMapping();
	///Creates a mapping from exon coordinates to ENST transcript identifier
	QMap<QByteArray, QByteArrayList> getExonTranscriptMapping();
	///Returns a list of all expression values for a given gene symbol
	QVector<double> getGeneExpressionValues(const QByteArray& gene, int sys_id, const QString& tissue_type, bool log2=false);
	QVector<double> getGeneExpressionValues(const QByteArray& gene, QSet<int> cohort, bool log2=false);
	QVector<double> getGeneExpressionValues(const QByteArray& gene, QVector<int> cohort, bool log2=false);
	///Returns a list of all expression values for a given exon
	QVector<double> getExonExpressionValues(const BedLine& exon, QSet<int> cohort, bool log2=false);
	///Returns the expression values fo a single sample
	QMap<QByteArray, double> getGeneExpressionValuesOfSample(const QString& ps_id, bool allow_empty=false);
	///Returns the symbol<>id mapping of the gene expression helper table
	QMap<int, QByteArray> getGeneExpressionId2GeneMapping();
	QMap<QByteArray, int> getGeneExpressionGene2IdMapping();


	/***User handling functions ***/
	///Returns the database ID of the given user. If no user name is given, the current user from the environment is used. Throws an exception if the user is not in the NGSD user table.
	///If throw_if_false == false it returns -1 if user is not found
	int userId(QString user_name, bool only_active=false, bool throw_if_fails = true);
	///Returns the user login corresponding the given ID.
	QString userLogin(int user_id);
	///Returns the user name corresponding the given ID.
	QString userName(int user_id);
	///Returns the user email corresponding the given ID.
	QString userEmail(int user_id);
	///Replacement for passwords when they are shown in the GUI.
	static const QString& passwordReplacement();
	///Checks if the given user/password tuple is correct. If ok, returns an empty string. If not, returns an error message.
	QString checkPassword(QString user_name, QString password, bool only_active=true);
	///Sets the password for a NGSD user using a new random salt.
	void setPassword(int user_id, QString password);
	///Return a role for a given user.
	QString getUserRole(int user_id);

	///Checks if the user has one of the given roles.
	bool userRoleIn(QString user, QStringList roles);
	///Checks if the user can access the processed sample. Use for users with role 'restricted_user' only, or it will be slow because the user role has to be checked every time. Uses caching for massive speed-up.
	bool userCanAccess(int user_id, int ps_id);

	/*** Main NGSD functions ***/
	///Search for processed samples
	DBTable processedSampleSearch(const ProcessedSampleSearchParameters& params);
	///Returns sample data from the database.
	SampleData getSampleData(const QString& sample_id);
	///Returns processed sample data from the database.
	ProcessedSampleData getProcessedSampleData(const QString& processed_sample_id);
	///Returns the normal processed sample corresponding to a tumor processed sample, or "" if no normal samples is defined.
	QString normalSample(const QString& processed_sample_id);

	///Returns the corresponding sample id(s) with relation 'same sample' (mode:SAME_SAMPLE) or 'same patient' and 'same patient' (mode: SAME_PATIENT). Uses the cache to avoid database queries.
	/// (Does not contain the provided sample itself)
	const QSet<int>& sameSamples(int sample_id, SameSampleMode mode);
	///Returns related sample id(s). Uses the cache to avoid database queries.
	const QSet<int>& relatedSamples(int sample_id);
	///Return a list of sample ids (not name) which have a (specific) relation of the given sample id. If relation is "", all relations are reported.
	QSet<int> relatedSamples(int sample_id, const QString& relation, QString sample_type="");
	///Adds a new sample relation to the database;
	void addSampleRelation(const SampleRelation& rel, bool error_if_already_present=false);

	///Returns sample disease details from the database.
	QList<SampleDiseaseInfo> getSampleDiseaseInfo(const QString& sample_id, QString only_type="");
	///Sets the disease details of a sample.
	void setSampleDiseaseInfo(const QString& sample_id, const QList<SampleDiseaseInfo>& disease_info);
	///Adds a disease entry to a sample.
	void addSampleDiseaseInfo(const QString& sample_id, const SampleDiseaseInfo& entry);
	///Sets the disease group/status of a sample.
	void setSampleDiseaseData(const QString& sample_id, const QString& disease_group, const QString& disease_status);
	///Returns all phenotypes associated to the given sample
	PhenotypeList samplePhenotypes(const QString& sample_id, bool throw_on_error=false);

	///Returns the processing system ID from the name (tests short and long name). Returns -1 or throws a DatabaseException if not found.
	int processingSystemId(QString name, bool throw_if_fails = true);
	///Returns the processing system ID from the processed sample name. Throws a DatabaseException if processed sample does not exist;
	int processingSystemIdFromProcessedSample(QString ps_name);
	///Returns the processing system information for a processed sample.
	ProcessingSystemData getProcessingSystemData(int sys_id);

	///Returns a path (including filename) for the processing system target region file. Returns an empty string if unset.
	QString processingSystemRegionsFilePath(int sys_id);
	///Returns the processing system target region file.
	BedFile processingSystemRegions(int sys_id, bool ignore_if_missing);

	///Returns a path (including filename) for the processing system amplicon region file.  Returns an empty string if unset.
	QString processingSystemAmpliconsFilePath(int sys_id);
	///Returns the processing system amplicon region file.
	BedFile processingSystemAmplicons(int sys_id, bool ignore_if_missing);

	///Returns a path (including filename) for the processing system genes. Returns an empty string if unset.
	QString processingSystemGenesFilePath(int sys_id);
	///Returns the processing system genes.
	GeneSet processingSystemGenes(int sys_id, bool ignore_if_missing);

	///Retuns the list of sub-panel names.
	QStringList subPanelList(bool archived);
	///Returns the subpanel target region file.
	BedFile subpanelRegions(QString name);
	///Returns the subpanel genes.
	GeneSet subpanelGenes(QString name);

	///Returns all coresponding cfDNA panel info for a given processed sample
	QList<CfdnaPanelInfo> cfdnaPanelInfo(const QString& processed_sample_id, int processing_system_id = -1);
	///stores a cfDNA panel in the NGSD
	void storeCfdnaPanel(const CfdnaPanelInfo& panel_info, const QByteArray& bed_content, const QByteArray& vcf_content);
	///Returns the BED file of a given cfDNA panel
	BedFile cfdnaPanelRegions(int id);
	///Returns the VCF of a given cfDNA panel
	VcfFile cfdnaPanelVcf(int id);
	///Returns the BED file of the removed regions of a given cfDNA panel
	BedFile cfdnaPanelRemovedRegions(int id);
	///Updates the regions which where removed by the panel provider
	void setCfdnaRemovedRegions(int id, BedFile removed_regions);
	///Returns all available cfDNA gene entries
	QList<CfdnaGeneEntry> cfdnaGenes();
	///Returns the ID SNPs of a processing system as VCF
	VcfFile getIdSnpsFromProcessingSystem(int sys_id, const BedFile& target_region, bool tumor_only = false, bool throw_on_fail = true);

	///Returns all QC terms of the sample
	QCCollection getQCData(const QString& processed_sample_id);
	///Returns all values for a QC term (from sample of the same processing system)
	QVector<double> getQCValues(const QString& accession, const QString& processed_sample_id);
	///Returns KASP data. Thows a DatabaseException if no valid KASP was performed for the sample.
	KaspData kaspData(const QString& processed_sample_id);

	///Returns the next processing ID for the given sample.
	QString nextProcessingId(const QString& sample_id);

	///Returns classification information
	ClassificationInfo getClassification(const Variant& variant);
	///Sets the classification of a variant in the NGSD.
	void setClassification(const Variant& variant, const VariantList& variant_list, ClassificationInfo info);
	///Returns somatic classification information
	ClassificationInfo getSomaticClassification(const Variant& variant);
	///Sets the somatic classification of a variant in the NGSD.
	void setSomaticClassification(const Variant& variant, ClassificationInfo info);

	SomaticViccData getSomaticViccData(const Variant& variant, bool throw_on_fail = true);
	int getSomaticViccId(const Variant& variant);
	void setSomaticViccData(const Variant& variant, const SomaticViccData& vicc_data, QString user_name);

	///Returns a list of all somatic pathways.
	QByteArrayList getSomaticPathways();
	///Returns a list of somatic pathways that contain the given gene.
	QByteArrayList getSomaticPathways(QByteArray gene_symbol);
	///Returns a list of genes contained in the given pathway
	GeneSet getSomaticPathwayGenes(QByteArray pathway_name);


	///Returns the NGSD id of a somatic gene role
	int getSomaticGeneRoleId(QByteArray gene_symbol);
	///Returns the somatic gene role data for a gene. If there is no gene role definition and throw_on_fail=false, a invalid SomaticGeneRole instance is returned.
	SomaticGeneRole getSomaticGeneRole(QByteArray gene, bool throw_on_fail = false);
	///stores/updates somatic gene role data. "gene_role" has to contain valid gene
	void setSomaticGeneRole(const SomaticGeneRole& gene_role);
	///delete somatic gene role data for certain gene
	void deleteSomaticGeneRole(QByteArray gene);


	///Adds a variant publication
	int addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details, int user_id=-1);
	int addVariantPublication(QString processed_sample, const CopyNumberVariant& cnv, QString database, QString classification, QString details, int user_id=-1);
	int addVariantPublication(QString processed_sample, const BedpeLine& sv, const BedpeFile& svs, QString database, QString classification, QString details, int user_id=-1);
	int addManualVariantPublication(QString sample_name, QString database, QString classification, QString details, int user_id=-1);
	///Returns variant publication data as text
	QString getVariantPublication(QString filename, const Variant& variant);
	QString getVariantPublication(QString filename, const CopyNumberVariant& cnv);
	QString getVariantPublication(QString filename, const BedpeLine& sv, const BedpeFile& svs);
	///Updates ClinVar result of a variant publication
	void updateVariantPublicationResult(int variant_publication_id, QString result);
	///Flag a variant publication as replaced
	void flagVariantPublicationAsReplaced(int variant_publication_id);
	///Link two variant publications (comp. het. variant)
	void linkVariantPublications(int variant_publication_id1, int variant_publication_id2);
	///update submission status of uploaded variants from ClinVar
	QPair<int, int> updateClinvarSubmissionStatus(bool test_run);
	///returns the submission status of a given ClinVar submission Id
	ClinvarSubmissionStatus getSubmissionStatus(const QString& submission_id, bool test_run);

	///Returns the comment of a variant in the NGSD.
	QString comment(const Variant& variant);
	///Sets the comment of a variant in the NGSD.
	void setComment(const Variant& variant, const QString& text);

	///Returns the diagnostic status of a sample. If there is no such entry, a default-constructed instance of DiagnosticStatusData is returned.
	DiagnosticStatusData getDiagnosticStatus(const QString& processed_sample_id);
	///Sets the diagnostic status. Throws an exception, if the processed sample is not in the database. If unset, the user name is taken from the environment.
	void setDiagnosticStatus(const QString& processed_sample_id, DiagnosticStatusData status);
	///Returns if the report configuration database ID, or -1 if not present.
	int reportConfigId(const QString& processed_sample_id);
	///Returns if the report configuration text summary.
	QString reportConfigSummaryText(const QString& processed_sample_id);
	///Returns if the report configuration is finalized.
	bool reportConfigIsFinalized(int id);
	///Returns the report configuration for a processed sample, throws an error if it does not exist.
	QSharedPointer<ReportConfiguration> reportConfig(int id, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs);
	///Sets/overwrites the report configuration for a processed sample. Returns its database primary key. The variant list is needed to determine the annotation column indices.
	int setReportConfig(const QString& processed_sample_id, QSharedPointer<ReportConfiguration> config, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs);
	///Finalizes the report configuration. It cannot be modified afterwards!
	void finalizeReportConfig(int id, int user_id);
	///Deletes a report configuration.
	void deleteReportConfig(int id);
	///Returns the report variant configuration variant for a given id
	ReportVariantConfiguration reportVariantConfiguration(int id, VariantType type, QStringList& messages, const VariantList& variants=VariantList(), const CnvList& cnvs=CnvList(), const BedpeFile& svs=BedpeFile());

	///Returns the varint evaluation sheet data for a given processed sample id
	EvaluationSheetData evaluationSheetData(const QString& processed_sample_id, bool throw_if_fails = true);
	///Stores a given EvaluationSheetData in the NGSD (return table id)
	int storeEvaluationSheetData(const EvaluationSheetData& evaluation_sheet_data, bool overwrite_existing_data = false);

	///Returns the report config creation data (user/date) for somatic reports
	SomaticReportConfigurationData somaticReportConfigData(int id);

	///Returns database ID of somatic report configuration, -1 if not present
	int somaticReportConfigId(QString t_ps_id, QString n_ps_id);
	///Sets/overwrites somatic report configuration for tumor-normal processed sample pair
	int setSomaticReportConfig(QString t_ps_id, QString n_ps_id, const SomaticReportConfiguration& config, const VariantList& snvs, const CnvList& cnvs, const VariantList& germl_snvs, QString user_name);
	///Removes a somatic report configuration from NGSD, including its variant and cnv configurations
	void deleteSomaticReportConfig(int id);
	///Retrieve somatic report configuration using tumor and normal processed sample ids
	SomaticReportConfiguration somaticReportConfig(QString t_ps_id, QString n_ps_id, const VariantList& snvs, const CnvList& cnvs, const VariantList& germline_snvs, QStringList& messages);
	///set upload time of somatic XML report to current timestamp
	void setSomaticMtbXmlUpload(int report_id);

	///Sets processed sample quality
	void setProcessedSampleQuality(const QString& processed_sample_id, const QString& quality);

	///Returns the germline gene information for a HGNC-approved gene symbol
	GeneInfo geneInfo(QByteArray symbol);
	///Sets the germline gene information for a HGNC-approved gene symbol (not gnomAD o/e scores, because it is read-only)
	void setGeneInfo(GeneInfo info);

	///Returns the job id of the last single sample analysis or -1 if no analysis was performed.
	int lastAnalysisOf(QString processed_sample_id);
	///Returns information about an analysis job
	AnalysisJob analysisInfo(int job_id, bool throw_if_fails = true);
	///Queues an analysis.
	void queueAnalysis(QString type, bool high_priority, QStringList args, QList<AnalysisJobSample> samples);
	///Canceles an analysis. Returns 'true' if it was canceled and 'false' if it was not running anymore.
	bool cancelAnalysis(int job_id);
	///Deletes the analysis job record. Returns 'true' if a job was deleted, i.e. a job with the given ID existed.
	bool deleteAnalysis(int job_id);
	///Returns the folder of an analysis job.
	QString analysisJobFolder(int job_id);
	///Return metdata for the logs of an analysis job by its id
	FileInfo analysisJobLatestLogInfo(int job_id);
	///Returns the GSVar file of an analysis job.
	QString analysisJobGSvarFile(int job_id);

	///Adds a gap for a sample and returns the gap ID.
	int addGap(int ps_id, const Chromosome& chr, int start, int end, const QString& status);
	///Returns the gap ID. If no matching gap is found, -1 is returned.
	int gapId(int ps_id, const Chromosome& chr, int start, int end);
	///Updates the status of a gap.
	void updateGapStatus(int id, const QString& status);
	///Add a comment to the gap history.
	void addGapComment(int id, const QString& comment);

	///Returns variant calling inforation if available
	VariantCallingInfo variantCallingInfo(QString ps_id);
	///Returns quality metric for a CNV callsets (all metrics for a single sample)
	QHash<QString, QString> cnvCallsetMetrics(int callset_id);
	///Returns quality metric values for a given metric for all samples of a given processing system
	QVector<double> cnvCallsetMetrics(QString processing_system_id, QString metric_name);

	///Parses OBO file and updates QC term data
	void updateQC(QString obo_file, bool debug=false);

	///Checks for missing or inconsistent meta data of a sample.
	QHash<QString, QStringList> checkMetaData(const QString& ps_id, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs);

	///Checks for errors/inconsistencies and fixes them if @p fix_errors is set.
	void maintain(QTextStream* messages, bool fix_errors);

	///Returns the content of a NovaSeqX Plus SampleSheet for a given run
	QString createSampleSheet(int run_id, QStringList& warnings);

signals:
	void initProgress(QString text, bool percentage);
	void updateProgress(int percentage);

protected:
	///Copy constructor "declared away".
	NGSD(const NGSD&) = delete;
	void fixGeneNames(QTextStream* messages, bool fix_errors, QString table, QString column);
	static QString escapeForSql(const QString& text);

	///Returns the maxiumn allele frequency of a variant.
	static double maxAlleleFrequency(const Variant& v, QList<int> af_column_index);

	///Returns the target region folder.
	static QString getTargetFilePath();

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool test_db_;

	///Caching functionality (static)
	struct Cache
	{
		Cache();

		QMap<QString, TableInfo> table_infos;
		QHash<int, QSet<int>> same_samples;
		QHash<int, QSet<int>> same_patients;
		QHash<int, QSet<int>> related_samples;
		GeneSet approved_gene_names;
		QHash<QByteArray, int> gene2id;
		QMap<QString, QStringList> enum_values;
		QMap<QByteArray, QByteArray> non_approved_to_approved_gene_names;
		QHash<int, Phenotype> phenotypes_by_id;
		QHash<QByteArray, int> phenotypes_accession_to_id;

		TranscriptList gene_transcripts;
		ChromosomalIndex<TranscriptList> gene_transcripts_index;
		QHash<int, int> gene_transcripts_id2index; //NGSD transcript id > index in 'gene_transcripts'
		QHash<QByteArray, QSet<int>> gene_transcripts_symbol2indices; //gene symbol > indices in 'gene_transcripts'

		//gene expression
		QMap<int, QByteArray> gene_expression_id2gene;
		QMap<QByteArray, int> gene_expression_gene2id;
	};
	static Cache& getCache();
	void clearCache();
	void initTranscriptCache();
	void initGeneExpressionCache();
};

#endif // NGSD_H
