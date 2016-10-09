#ifndef NGSD_H
#define NGSD_H

#include "cppNGSD_global.h"
#include <QVariant>
#include <QVariantList>
#include <QSharedPointer>
#include <QTextStream>
#include "VariantList.h"
#include "BedFile.h"
#include "QCCollection.h"
#include "SqlQuery.h"

/// Germline gene information.
struct CPPNGSDSHARED_EXPORT GeneInfo
{
	//gene symbol
	QString symbol;
	//gene inheritance mode
	QString inheritance;
	//comments
	QString comments;

	//notice about the symbol based on HGNC data (unknown symbol, previous symbol, etc.)
	QString notice;
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
	int geneToApprovedID(const QString& gene);
	///Returns the gene symbol for a gene ID
	QString geneSymbol(int id);
	///Returns the the approved/original gene symbol and a status message.
	QPair<QString, QString> geneToApproved(const QString& gene);
	///Returns previous symbols of a gene.
	QStringList previousSymbols(QString symbol);
	///Returns aliases of a gene.
	QStringList synonymousSymbols(QString symbol);
	///Returns the genes overlapping a regions (extended by some bases)
	QStringList genesOverlapping(const Chromosome& chr, int start, int end, int extend=0);
	///Returns the genes overlapping a regions (extended by some bases)
	QStringList genesOverlappingByExon(const Chromosome& chr, int start, int end, int extend=0);
	///Returns the chromosomal regions corrsponding to the given genes. Messages about unknown gene symbols etc. are written to the steam, if given.
	BedFile genesToRegions(QStringList genes, QString source, QString mode, QTextStream* messages = nullptr);
	///Returns longest coding transcript name and region.
	void longestCodingTranscript(int id, QString source, QString& name, BedFile& region, Chromosome& chr);

	/*** phenotype handling (HPO) ***/
	///Returns the phenotypes of a gene
	QStringList phenotypes(QString symbol);
	///Returns all phenotypes matching the given search terms (or all terms if no search term is given)
	QStringList phenotypes(QStringList terms);
	///Returns all genes associated to a phenotype
	QStringList phenotypeToGenes(QString phenotype, bool recursive);

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
	///Returns the NGSD ID for a variant. Returns '-1' or throws an exception if the ID cannot be determined.
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
	///Returns the processing system information for the sample, or an empty string if it could not be detected.
	enum SystemType {SHORT, LONG, BOTH, TYPE, FILE};
	QString getProcessingSystem(const QString& filename, SystemType type);
	///Returns all processing systems (long name) and the corresponding target regions.
	QMap<QString, QString> getProcessingSystems(bool skip_systems_without_roi, bool windows_paths);
	///Returns the genome build
	QString getGenomeBuild(const QString& filename);
	///Returns all QC terms of the sample
	QCCollection getQCData(const QString& filename);
	///Returns all values for a QC term (from sample of the same processing system)
	QVector<double> getQCValues(const QString& accession, const QString& filename);
	///Returns the next processing ID for the given sample.
	QString nextProcessingId(const QString& sample_id);

	///Annotates (or re-annotates) the variant list with current NGSD information.
	void annotate(VariantList& variants, QString filename);
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

	///Returns the comment of a variant in the NGSD.
	QString comment(const QString& filename, const Variant& variant);
	///Sets the comment of a variant in the NGSD.
	void setComment(const QString& filename, const Variant& variant, const QString& text);

	///Sets the report status of all variants in the NGSD.
	void setReportVariants(const QString& filename, const VariantList& variants, QSet<int> selected_indices);

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

	///Returns the germline gene information for a HGNC-approved gene symbol
	GeneInfo geneInfo(QString symbol);
	///Sets the germline gene information for a HGNC-approved gene symbol
	void setGeneInfo(GeneInfo info);

	///Returns the NGSD URL corresponding to a variant. Or an empty string if the variant/sample is not in the DB.
	QString url(const QString& filename, const Variant& variant);
	///Returns the NGSD URL corresponding to a processed sample. Or an empty string if the sample is not in the DB.
	QString url(const QString& filename);
	///Returns the NGSD seach URL including the search term.
	QString urlSearch(const QString& search_term);

	///Returns the target file path (or sub-panel folder)
	static QString getTargetFilePath(bool subpanels = false, bool windows = true);

signals:
	void initProgress(QString text, bool percentage);
	void updateProgress(int percentage);

protected:
	///Copy constructor "declared away".
	NGSD(const NGSD&);

	///Adds a sample-specific string column with empty entries. Returns the index of the added column.
	int addColumn(VariantList& variants, QString name, QString description);
	///Removes a column if it is present. Returns if the column was present.
	bool removeColumnIfPresent(VariantList& variants, QString name, bool exact_name_match);

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool test_db_;
	bool is_open_;
};


#endif // NGSD_H
