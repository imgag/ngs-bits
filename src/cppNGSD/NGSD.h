#ifndef NGSD_H
#define NGSD_H

#include "cppNGSD_global.h"
#include <QVariant>
#include <QVariantList>
#include <QSharedPointer>
#include <QTextStream>
#include <QDateTime>
#include "VariantList.h"
#include "BedFile.h"
#include "Transcript.h"
#include "QCCollection.h"
#include "SqlQuery.h"
#include "GeneSet.h"
#include "Phenotype.h"
#include "Helper.h"
#include "DBTable.h"

///General database field information.
struct CPPNGSDSHARED_EXPORT TableFieldInfo
{
	enum Type
	{
		BOOL, INT, FLOAT, TEXT, VARCHAR, ENUM, DATE, FK
	};

	int index = -1;
	QString name;
	Type type;
	QVariant type_restiction; //length of VARCHAR and value of ENUM
	bool nullable;
	QString default_value;
	bool primary_key;
	QString fk_table;
	QString fk_field;
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
		const TableFieldInfo& fieldInfo(QString field) const
		{
			foreach(const TableFieldInfo& entry, field_infos_)
			{
				if (entry.name==field)
				{
					return entry;
				}
			}

			THROW(DatabaseException, "Field '" + field + "' not found in NGSD table '" + table_ + "'!");
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

	bool operator==(const AnalysisJobSample& rhs)
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


///Sample information.
struct CPPNGSDSHARED_EXPORT SampleData
{
	QString name;
	QString name_external;
	QString gender;
	QString quality;
	QString comments;
	QString disease_group;
	QString disease_status;
	bool is_tumor;
	bool is_ffpe;
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
	QString quality;
	QString gender;
	QString comments;
	QString project_name;
	QString run_name;
	QString normal_sample_name;
};

///Processing system information.
struct CPPNGSDSHARED_EXPORT ProcessingSystemData
{
	QString name;
	QString name_short;
	QString target_file;
	QString adapter1_p5;
	QString adapter2_p7;
	bool shotgun;
	QString type;
	QString genome;
};

/// Germline gene information.
struct CPPNGSDSHARED_EXPORT GeneInfo
{
	//gene symbol
	QString symbol;
	//gene name
	QString name;
	//gene inheritance mode
	QString inheritance;
	//ExAC pLI score (default is NULL)
	QString exac_pli;
	//comments
	QString comments;

	//notice about the symbol based on HGNC data (unknown symbol, previous symbol, etc.)
	QString notice;

	//returns the main gene information as a string
	QString toString()
	{
		return symbol + " (inh=" + inheritance + " pLI=" + exac_pli + ")";
	}
};

///Variant validation information
struct CPPNGSDSHARED_EXPORT ValidationInfo
{
	QString status;
	QString type;
	QString comments;
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
	QString genes_causal = "";
	QString inheritance_mode = "n/a";
	QString genes_incidental = "";
	QString comments = "";

	//meta data
	QString user;
	QDateTime date;
};

/// NGSD accessor.
class CPPNGSDSHARED_EXPORT NGSD
		: public QObject
{
Q_OBJECT

public:
	///Default constructor that connects to the DB
	NGSD(bool test_db = false);
	///Destructor.
	~NGSD();
	///Returns if the database connection is open
	bool isOpen() const;

	///Returns the table list.
	QStringList tables() const;
	///Returns information about all fields of a table.
	const TableInfo& tableInfo(QString table) const;

	///Creates an table with data from an SQL query.
	DBTable createTable(QString table, QString query, int pk_col_index=0);

	///Creates database tables and imports initial data (password is required for production database if it is not empty)
	void init(QString password="");

	/*** General database functionality ***/
	///Executes an SQL query and returns the single return value.
	///If no values are returned an error thrown or a default-constructed QVariant is returned (depending on @p empty_is_ok).
	///If more than one value is returned a DatabaseError is thrown.
	QVariant getValue(const QString& query, bool no_value_is_ok=true);
	///Executes an SQL query and returns the value list.
	QStringList getValues(const QString& query);
	///Returns a SqlQuery object on the NGSD for custom queries.
	inline SqlQuery getQuery() const
	{
		return SqlQuery(*db_);
	}
	///Executes all queries from a text file.
	void executeQueriesFromFile(QString filename);

	///Returns all possible values for a enum column.
	QStringList getEnum(QString table, QString column);
	///Checks if a table exists.
	void tableExists(QString table);
	///Checks if a table is empty.
	bool tableEmpty(QString table);
	///Clears all contents from a table.
	void clearTable(QString table);

	/*** gene/transcript handling ***/
	///Returns the gene ID, or -1 if none approved gene name could be found. Checks approved symbols, previous symbols and synonyms.
	int geneToApprovedID(const QByteArray& gene);
	///Returns the gene symbol for a gene ID
	QByteArray geneSymbol(int id);
	///Returns the the approved gene symbol or "" if it could not be determined.
	QByteArray geneToApproved(QByteArray gene, bool return_input_when_unconvertable=false);
	///Returns the the approved gene symbols.
	GeneSet genesToApproved(GeneSet genes, bool return_input_when_unconvertable=false);
	///Returns the the approved/original gene symbol and a status message.
	QPair<QString, QString> geneToApprovedWithMessage(const QString& gene);
	///Returns previous symbols of a gene.
	GeneSet previousSymbols(int id);
	///Returns aliases of a gene.
	GeneSet synonymousSymbols(int id);
	///Returns the genes overlapping a regions (extended by some bases)
	GeneSet genesOverlapping(const Chromosome& chr, int start, int end, int extend=0);
	///Returns the genes overlapping a regions (extended by some bases)
	GeneSet genesOverlappingByExon(const Chromosome& chr, int start, int end, int extend=0);
	///Returns the chromosomal regions corresponding to the given genes. Messages about unknown gene symbols etc. are written to the steam, if given.
	BedFile genesToRegions(const GeneSet& genes, Transcript::SOURCE source, QString mode, bool fallback = false, bool annotate_transcript_names = false, QTextStream* messages = nullptr);
	///Returns transcripts of a gene (if @p coding_only is set, only coding transcripts and regions are returned).
	QList<Transcript> transcripts(int gene_id, Transcript::SOURCE source, bool coding_only);
	///Returns longest coding transcript of a gene.
	Transcript longestCodingTranscript(int gene_id, Transcript::SOURCE source, bool fallback_ensembl=false, bool fallback_ensembl_nocoding=false);
	///Returns the list of all approved gene names
	const GeneSet& approvedGeneNames();

	/*** phenotype handling (HPO) ***/
	///Returns the phenotype for a given HPO accession.
	Phenotype phenotypeByAccession(const QByteArray& accession, bool throw_on_error=true);
	///Returns the phenotypes of a gene
	QList<Phenotype> phenotypes(const QByteArray& symbol);
	///Returns all phenotypes matching the given search terms (or all terms if no search term is given)
	QList<Phenotype> phenotypes(QStringList search_terms);
	///Returns all genes associated to a phenotype
	GeneSet phenotypeToGenes(const Phenotype& phenotype, bool recursive);
	///Returns all child terms of the given phenotype
	QList<Phenotype> phenotypeChildTems(const Phenotype& phenotype, bool recursive);

	/*** Base functionality for file/variant processing ***/
	///Returns the processed sample name for an ID.
	QString processedSampleName(const QString& ps_id, bool throw_if_fails = true);
	///Returns the NGSD sample ID file name. Throws an exception if it could not be determined.
	QString sampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD processed sample ID from a file name or processed sample name. Throws an exception if it could not be determined.
	QString processedSampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the default folder for a processed sample from file name or processed sample name. Throws an exception if it could not be determined.
	enum PathType {PROJECT_FOLDER, SAMPLE_FOLDER, BAM, GSVAR, VCF};
	QString processedSamplePath(const QString& processed_sample_id, PathType type);
	///Adds a variant to the NGSD, if not already present. Returns the variant ID.
	QString addVariant(const Variant& variant, const VariantList& vl);
	///Returns the NGSD ID for a variant. Returns '' or throws an exception if the ID cannot be determined.
	QString variantId(const Variant& variant, bool throw_if_fails = true);
	///Returns the database ID of the user as a string. Throws an exception if the user is not in the NGSD user table.
	QString userId(QString user_name=Helper::userName());

	/*** Main NGSD functions ***/
	///Returns sample data from the database.
	SampleData getSampleData(const QString& sample_id);
	///Returns processed sample data from the database.
	ProcessedSampleData getProcessedSampleData(const QString& processed_sample_id);
	///Returns the normal processed sample corresponding to a tumor processed sample, or "" if no normal samples is defined.
	QString normalSample(const QString& processed_sample_id);

	///Returns sample disease details from the database.
	QList<SampleDiseaseInfo> getSampleDiseaseInfo(const QString& sample_id, QString only_type="");
	///Sets the disease details of a sample.
	void setSampleDiseaseInfo(const QString& sample_id, const QList<SampleDiseaseInfo>& disease_info);
	///Sets the disease group/status of a sample.
	void setSampleDiseaseData(const QString& sample_id, const QString& disease_group, const QString& disease_status);

	///Returns the processing system information for a processed sample.
	ProcessingSystemData getProcessingSystemData(const QString& processed_sample_id, bool windows_path);
	///Returns all processing systems (long name) and the corresponding target regions.
	QMap<QString, QString> getProcessingSystems(bool skip_systems_without_roi, bool windows_paths);
	///Returns all QC terms of the sample
	QCCollection getQCData(const QString& processed_sample_id);
	///Returns all values for a QC term (from sample of the same processing system)
	QVector<double> getQCValues(const QString& accession, const QString& processed_sample_id);
	///Returns the next processing ID for the given sample.
	QString nextProcessingId(const QString& sample_id);

	///Precalcualtes genotype counts for all variants.
	void precalculateGenotypeCounts(QTextStream* messages = nullptr, int progress_interval = -1);
	///Annotates (or re-annotates) the variant list with current NGSD information. If @p roi is non-empty, only the variants in the target region are annotated. If max_af is greater than 0, only variants with AF<=cutoff are annotated.
	void annotate(VariantList& variants, QString filename, BedFile roi = BedFile(), double max_af = 0.0);
	///Annotates (or re-annotates) the variant list with current (somatic) NGSD information.
	void annotateSomatic(VariantList& variants, QString filename);

	///Returns validation status information
	ValidationInfo getValidationStatus(const QString& filename, const Variant& variant);
	///Sets that validation status of a variant in the NGSD. If unset, the user name is taken from the environment.
	void setValidationStatus(const QString& filename, const Variant& variant, const ValidationInfo& info, QString user_name=Helper::userName());

	///Returns classification information
	ClassificationInfo getClassification(const Variant& variant);
	///Sets the classification of a variant in the NGSD.
	void setClassification(const Variant& variant, ClassificationInfo info);

	///Adds a variant publication
	void addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details);
	///Returns variant publication data as text
	QString getVariantPublication(QString filename, const Variant& variant);

	///Returns the comment of a variant in the NGSD.
	QString comment(const Variant& variant);
	///Sets the comment of a variant in the NGSD.
	void setComment(const Variant& variant, const QString& text);

	///Returns the diagnostic status of a sample. If there is no such entry, a default-constructed instance of DiagnosticStatusData is returned.
	DiagnosticStatusData getDiagnosticStatus(const QString& processed_sample_id);
	///Sets the diagnostic status. Throws an exception, if the processed sample is not in the database. If unset, the user name is taken from the environment.
	void setDiagnosticStatus(const QString& processed_sample_id, DiagnosticStatusData status, QString user_name=Helper::userName());

	///Sets processed sample quality
	void setProcessedSampleQuality(const QString& processed_sample_id, const QString& quality);

	///Returns the germline gene information for a HGNC-approved gene symbol
	GeneInfo geneInfo(QByteArray symbol);
	///Sets the germline gene information for a HGNC-approved gene symbol (not ExAC pLI score, because it is read-only)
	void setGeneInfo(GeneInfo info);

	///Returns the NGSD URL corresponding to a variant. Or an empty string if the variant/sample is not in the DB.
	QString url(const QString& filename, const Variant& variant);
	///Returns the NGSD URL corresponding to a processed sample. Or an empty string if the sample is not in the DB.
	QString url(const QString& filename);
	///Returns the NGSD seach URL including the search term.
	QString urlSearch(const QString& search_term);

	///Returns the job id of the last single sample analysis or -1 if no analysis was performed.
	int lastAnalysisOf(QString processed_sample_id);
	///Returns information about an analysis job
	AnalysisJob analysisInfo(int job_id);
	///Queues an analysis.
	void queueAnalysis(QString type, bool high_priority, QStringList args, QList<AnalysisJobSample> samples, QString user_name=Helper::userName());
	///Canceles an analysis. Returns 'true' if it was canceled and 'false' if it was not running anymore.
	bool cancelAnalysis(int job_id, QString user_name=Helper::userName());
	///Deletes the analysis job record. Returns 'true' if a job was deleted, i.e. a job with the given ID existed.
	bool deleteAnalysis(int job_id);

	///Returns the target file path (or sub-panel folder)
	static QString getTargetFilePath(bool subpanels = false, bool windows = true);

	///Parses OBO file and updates QC term data
	void updateQC(QString obo_file, bool debug=false);

	///Checks for errors/inconsistencies and fixes them if @p fix_errors is set.
	void maintain(QTextStream* messages, bool fix_errors);

signals:
	void initProgress(QString text, bool percentage);
	void updateProgress(int percentage);

protected:
	///Copy constructor "declared away".
	NGSD(const NGSD&) = delete;
	void fixGeneNames(QTextStream* messages, bool fix_errors, QString table, QString column);

	///Returns the maxiumn allele frequency of a variant.
	static double maxAlleleFrequency(const Variant& v, QList<int> af_column_index);

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool test_db_;
	bool is_open_;

	static QMap<QString, TableInfo> infos_;
};


#endif // NGSD_H
