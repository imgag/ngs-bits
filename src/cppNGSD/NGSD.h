#ifndef NGSD_H
#define NGSD_H

#include "cppNGSD_global.h"
#include <QVariant>
#include <QVariantList>
#include "VariantList.h"
#include "BedFile.h"
#include "QCCollection.h"
#include "SqlQuery.h"

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

	///Initializes the database (password is required for production database if it is not empty)
	void init(QString password="");

	/*** General database functionality ***/
	///Executes an SQL query and returns the single return value.
	///If no values are returned an error thrown or a default-constructed QVariant is returned (depending on @p empty_is_ok).
	///If more than one value is returned a DatabaseError is thrown.
	QVariant getValue(const QString& query, bool no_value_is_ok=true);
	///Executes an SQL query and returns the return value list.
	QVariantList getValues(const QString& query);
	///Executes an SQL query and returns the return value list.
	QStringList getValuesAsString(const QString& query);
	///Returns a SqlQuery object on the NGSD for custom queries.
	inline SqlQuery getQuery() const
	{
		return SqlQuery(db_);
	}
	///Executes all queries from a text file.
	void executeQueriesFromFile(QString filename);

	///Returns all possible values for a enum column.
	QStringList getEnum(QString table, QString column);
	///Checks if a table exists
	void tableExists(QString table);
	///Checks if a table is empty
	bool tableEmpty(QString table);

	/*** gene/transcript handling ***/
	///Returns the gene ID, or -1 if none approved gene name could be found. Checks approved symbols, previous symbols and synonyms.
	int geneToApprovedID(const QByteArray& gene);
	///Returns the genes overlapping a regions (extended by some bases)
	QStringList genesOverlapping(QByteArray chr, int start, int end, int extend=0);
	///Returns the chromosomal regions corrsponding to the given genes.
	BedFile genesToRegions(QStringList genes, QString source, QString mode, bool messages=false);


	/*** Base functionality for file/variant processing ***/
	///Returns the sample name for a file name, e.g. 'GS120159' for '/some/path/GS120159_01.bam'. Throws an exception if the file name does not start with a valid name.
	static QString sampleName(const QString& filename, bool throw_if_fails = true);
	///Returns the processed sample name for a file name, e.g. 'GS120159_01' for '/some/path/GS120159_01.bam'. Throws an exception if the file name does not start with a valid name.
	static QString processedSampleName(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD sample ID file name. Throws an exception if it could not be determined.
	QString sampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD processed sample ID for a file name. Throws an exception if it could not be determined.
	QString processedSampleId(const QString& filename, bool throw_if_fails = true);
	///Returns the NGSD ID for a variant. Returns '-1' or throws an exception if the ID cannot be determined.
	QString variantId(const Variant& variant, bool throw_if_not_found = true);
	///Returns the ID of the current user as a string. Throws an exception if the user is not in the NGSD user table.
	QString userId();

	/*** Main NGSD functions ***/
	///Returns the external sample name given the file name.
	QString getExternalSampleName(const QString& filename);
	///Returns the processing system name and short name of the sample, or an empty string if it could not be detected.
	enum SystemType {SHORT, LONG, BOTH};
	QString getProcessingSystem(const QString& filename, SystemType type);
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
	QPair<QString, QString> getValidationStatus(const QString& filename, const Variant& variant);
	///Sets that validation status of a variant in the NGSD.
	void setValidationStatus(const QString& filename, const Variant& variant, const QString& status, const QString& comment);

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

	///Returns the NGSD URL corresponding to a variant. Or an empty string if the variant/sample is not in the DB.
	QString url(const QString& filename, const Variant& variant);
	///Returns the NGSD URL corresponding to a processed sample. Or an empty string if the sample is not in the DB.
	QString url(const QString& filename);
	///Returns the NGSD seach URL including the search term.
	QString urlSearch(const QString& search_term);

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
	QSqlDatabase db_;
	bool test_db_;
};

#endif // NGSD_H
