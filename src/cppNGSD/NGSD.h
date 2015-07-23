#ifndef NGSD_H
#define NGSD_H

#include "cppNGSD_global.h"
#include <QVariant>
#include <QVariantList>
#include "VariantList.h"
#include "QCCollection.h"
#include "SqlQuery.h"

/// NGSD accessor.
class CPPNGSDSHARED_EXPORT NGSD
		: public QObject
{
Q_OBJECT

public:
	///Default constructor that connects to the DB.
	NGSD();
	///Destructor.
	~NGSD();
	///Connects to the database and throws a DatabaseError if the connection fails.
	bool connect();

	///Extracts the sample name from a file name, e.g. 'GS120159' from '/some/path/GS120159_01.bam'.
	static QString sampleName(const QString& filename);
	///Extracts the processed sample number from a file name, e.g. '1' from '/some/path/GS120159_01.bam'.
	static QString processedSampleNumber(const QString& filename);
	///Returns the external sample name given the file name.
	QString getExternalSampleName(const QString& filename);
	///Returns the processing system name and short name of the sample, or an empty string if it could not be detected.
	enum SystemType {SHORT, LONG, BOTH};
	QString getProcessingSystem(const QString& filename, SystemType type);
	///Returs validation status information
	QPair<QString, QString> getValidationStatus(const QString& filename, const Variant& variant);
	///Returns all QC terms of the sample
	QCCollection getQCData(const QString& filename);
	///Returns all values for a QC term (from sample of the same processing system)
	QVector<double> getQCValues(const QString& accession, const QString& filename);

	///Annotates (or re-annotates) the variant list with current NGSD information.
	void annotate(VariantList& variants, QString filename, QString ref_file, bool add_comment=false);
	///Annotates (or re-annotates) the variant list with current (somatic) NGSD information.
	void annotateSomatic(VariantList& variants, QString filename, QString ref_file);
	///Sets that validation status of a variant in the NGSD.
	void setValidationStatus(const QString& filename, const Variant& variant, const QString& status, const QString& comment);
	///Sets the classification of a variant in the NGSD.
	void setClassification(const Variant& variant, const QString& classification);
	///Returns the comment of a variant in the NGSD.
	QString comment(const QString& filename, const Variant& variant);
	///Sets the comment of a variant in the NGSD.
	void setComment(const QString& filename, const Variant& variant, const QString& text);
	///Sets the comment of a variant in the NGSD.
	void setReport(const QString& filename, const Variant& variant, bool in_report);

	///Returns all possible values for a enum column in the NGSD.
	QStringList getEnum(QString table, QString column);
	///Returns the diagnostic status of a sample (status, user, datetime, outcome), or an empty result if an error occurred.
	QStringList getDiagnosticStatus(const QString& filename);
	///Sets the diagnostic status.
	void setDiagnosticStatus(const QString& filename, QString status);
	///Sets the report outcome (use @p getDiagnosticStatus to get it).
	void setReportOutcome(const QString& filename, QString outcome);

	///Returns the NGSD URL corresponding to a variant. Or an empty string if the variant/sample is not in the DB.
	QString url(const QString& filename, const Variant& variant);
	///Returns the NGSD URL corresponding to a processed sample. Or an empty string if the sample is not in the DB.
	QString url(const QString& filename);
	///Returns the NGSD seach URL including the search term.
	QString urlSearch(const QString& search_term);

	///Executes an SQL query and returns the single return value.
	///If no values are returned an error thrown or a default-constructed QVariant is returned (depending on @p empty_is_ok).
	///If more than one value is returned a DatabaseError is thrown.
	QVariant getValue(const QString& query, bool no_value_is_ok=true);
	///Executes an SQL query and returns the return value list.
	QVariantList getValues(const QString& query);
	///Returns a SqlQuery object on the NGSD for custom queries.
	inline SqlQuery getQuery() const
	{
		return SqlQuery(db_);
	}

signals:
	initProgress(QString text, bool percentage);
	updateProgress(int percentage);

protected:
	///Copy constructor "declared away".
	NGSD(const NGSD&);

	///Adds a sample-specific string column with empty entries. Returns the index of the added column.
	int addColumn(VariantList& variants, QString name, QString description);
	///Removes a column if it is present. Returns if the column was present.
	bool removeColumnIfPresent(VariantList& variants, QString name, bool exact_name_match);

	///The database adapter
	QSqlDatabase db_;
};

#endif // NGSD_H
