#ifndef NGSD_H
#define NGSD_H

#include "cppNGSD_global.h"
#include <QVariant>
#include <QVariantList>
#include <QSharedPointer>
#include <QTextStream>

#include "VariantList.h"
#include "BedFile.h"
#include "Transcript.h"
#include "QCCollection.h"
#include "SqlQuery.h"
#include "GeneSet.h"

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
	QString comment;
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
	///Returns the the approved gene symbols. Unconvertaglb if it could not be determined.
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
	Transcript longestCodingTranscript(int gene_id, Transcript::SOURCE source);
	///Returns the list of all approved gene names
	const GeneSet& approvedGeneNames();

	/*** phenotype handling (HPO) ***/
	///Returns the phenotypes of a gene
	QStringList phenotypes(QByteArray symbol);
	///Returns all phenotypes matching the given search terms (or all terms if no search term is given)
	QStringList phenotypes(QStringList terms);
	///Returns all genes associated to a phenotype
	GeneSet phenotypeToGenes(QByteArray phenotype, bool recursive);
	///Returns the phenotype name for an phenotype ID. Throws an exception if the ID is not valid.
	QByteArray phenotypeIdToName(QByteArray id);

	/*** Base functionality for file/variant processing ***/
	///Returns the sample name for a file name, e.g. 'GS120159' for '/some/path/GS120159_01.bam'. Throws an exception if the file name does not start with a valid name.
	static QString sampleName(const QString& filename, bool throw_if_fails = true);
	///Returns the processed sample name for a file name, e.g. 'GS120159_01' for '/some/path/GS120159_01.bam'. Throws an exception if the file name does not start with a valid name.
	static QString processedSampleName(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD sample ID file name. Throws an exception if it could not be determined.
	QString sampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD processed sample ID from a file name or processed sample name. Throws an exception if it could not be determined.
	QString processedSampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the default folder for a processed sample from file name or processed sample name. Throws an exception if it could not be determined.
	enum PathType {FOLDER, BAM, GSVAR, VCF};
	QString processedSamplePath(const QString& filename, PathType type, bool throw_if_fails = true);
	///Returns the NGSD ID for a variant. Returns '' or throws an exception if the ID cannot be determined.
	QString variantId(const Variant& variant, bool throw_if_fails = true);
	///Returns the ID of the current user as a string. Throws an exception if the user is not in the NGSD user table.
	QString userId();

	/*** Main NGSD functions ***/
	///Returns the external sample name, or "n/a" the sample cannot be found in the database.
	QString getExternalSampleName(const QString& filename);
	///Returns the tumor status of a sample, or "n/a" the sample cannot be found in the database.
	QString sampleIsTumor(const QString& filename);
	///Returns the FFPE status of a sample, or "n/a" the sample cannot be found in the database.
	QString sampleIsFFPE(const QString& filename);
	///Returns the diease group associated to a sample.
	QString sampleDiseaseGroup(const QString& filename);
	///Sets the diease group associated to a sample.
	void setSampleDiseaseGroup(const QString& filename, const QString& disease_group);

	///Returns the diease status associated to a sample.
	QString sampleDiseaseStatus(const QString& filename);
	///Sets the diease status associated to a sample.
	void setSampleDiseaseStatus(const QString& filename, const QString& disease_status);

	///Returns the processing system information for the sample, or an empty string if it could not be detected.
	enum SystemType {SHORT, LONG, BOTH, TYPE, FILE};
	QString getProcessingSystem(const QString& filename, SystemType type);
	///Returns all processing systems (long name) and the corresponding target regions.
	QMap<QString, QString> getProcessingSystems(bool skip_systems_without_roi, bool windows_paths);
	///Returns the genome build
	QString getGenomeBuild(const QString& filename);
	///Returns the gender of a processed sample
	QString sampleGender(const QString& filename);
	///Returns all QC terms of the sample
	QCCollection getQCData(const QString& filename);
	///Returns all values for a QC term (from sample of the same processing system)
	QVector<double> getQCValues(const QString& accession, const QString& filename);
	///Returns the next processing ID for the given sample.
	QString nextProcessingId(const QString& sample_id);

	///Precalcualtes genotype counts for all variants.
	void precalculateGenotypeCounts(QTextStream* messages = nullptr, int progress_interval = -1);
	///Annotates (or re-annotates) the variant list with current NGSD information. If @p roi is non-empty, only the variants in the target region are annotated. If max_af is greater than 0, only variants with AF<=cutoff are annotated.
	void annotate(VariantList& variants, QString filename, BedFile roi = BedFile(), double max_af = 0.0);
	///Annotates (or re-annotates) the variant list with current (somatic) NGSD information.
	void annotateSomatic(VariantList& variants, QString filename);

	///Returns validation status information (status, comment)
	ValidationInfo getValidationStatus(const QString& filename, const Variant& variant);
	///Sets that validation status of a variant in the NGSD.
	void setValidationStatus(const QString& filename, const Variant& variant, const ValidationInfo& info);

	///Returns classification information (classification, comment)
	QPair<QString, QString> getClassification(const Variant& variant);
	///Sets the classification of a variant in the NGSD.
	void setClassification(const Variant& variant, const QString& classification, const QString& comment);

	///Adds a variant publication
	void addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details);
	///Returns variant publication data as text
	QString getVariantPublication(QString filename, const Variant& variant);

	///Returns the comment of a variant in the NGSD.
	QString comment(const Variant& variant);
	///Sets the comment of a variant in the NGSD.
	void setComment(const Variant& variant, const QString& text);

	///Returns the diagnostic status of a sample (status, user, datetime, outcome), or an empty result if an error occurred.
	QStringList getDiagnosticStatus(const QString& filename);
	///Sets the diagnostic status.
	void setDiagnosticStatus(const QString& filename, QString status);
	///Sets the report outcome (use @p getDiagnosticStatus to get it).
	void setReportOutcome(const QString& filename, QString outcome);

	///Returns processed sample quality
	QString getProcessedSampleQuality(const QString& filename, bool colored);
	///Sets processed sample quality
	void setProcessedSampleQuality(const QString& filename, QString quality);

	///Returns processed sample comment
	QString getProcessedSampleComment(const QString& filename);

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

	///Returns the target file path (or sub-panel folder)
	static QString getTargetFilePath(bool subpanels = false, bool windows = true);

	///Checks for errors/inconsistencies and fixes them if @p fix_errors is set.
	void maintain(QTextStream* messages, bool fix_errors);

signals:
	void initProgress(QString text, bool percentage);
	void updateProgress(int percentage);

protected:
	///Copy constructor "declared away".
	NGSD(const NGSD&);
	void fixGeneNames(QTextStream* messages, bool fix_errors, QString table, QString column);

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool test_db_;
	bool is_open_;
};


#endif // NGSD_H
