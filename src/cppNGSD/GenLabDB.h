#ifndef GENLABDB_H
#define GENLABDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSharedPointer>
#include "cppNGSD_global.h"
#include "PhenotypeList.h"
#include "NGSD.h"

/// GenLab database access (only views that serve as API)
class CPPNGSDSHARED_EXPORT GenLabDB
		: public QObject
{
Q_OBJECT

public:
	///Default constructor that connects to the DB
	GenLabDB();
	///Destructor.
	~GenLabDB();
	///Returns if the database connection is (still) open
	bool isOpen() const;

	///Returns if the database is available (i.e. the credentials are in the settings file)
	static bool isAvailable();

	///Returns HPO phenotypes of a sample
	PhenotypeList phenotypes(QString ps_name);

	///Returns Oprhanet identifiers of a sample
	QStringList orphanet(QString ps_name);

	///Returns the ICD10 diagnosis of a sample (tries sample name if processed sample name is not found)
	QStringList diagnosis(QString ps_name);

	///Returns the GenLab anamnese entry of a sample (tries sample name if processed sample name is not found)
	QStringList anamnesis(QString ps_name);

	///Returns tumor content of a sample (tries sample name if processed sample name is not found)
	QStringList tumorFraction(QString ps_name);

	///Returns the GenLab year of birth for a sample (tries sample name if processed sample name is not found)
	QString yearOfBirth(QString ps_name);

	///Returns the GenLab year of order entry for a sample (tries sample name if processed sample name is not found)
	QString yearOfOrderEntry(QString ps_name);

	///Returns disease group and disease status of a processed sample (tries sample name if processed sample name is not found)
	QPair<QString, QString> diseaseInfo(QString ps_name);

	///Returns SAP patient identifier (tries sample name if processed sample name is not found)
	QString sapID(QString ps_name);

	///Returns sample relations
	QList<SampleRelation> relatives(QString ps_name);

	///Returns the sample gender, or an empty string if unknown.
	QString gender(QString ps_name);

	///Returns the patient identifier, or an empty string if it could not be determined (tries sample name if processed sample name is not found)
	QString patientIdentifier(QString ps_name);

	///Returns all studies in GenLab.
	QStringList studies();

	///Returns the list of studies the sample is part of.
	QStringList studies(QString ps_name);

	///Returns a list of processed sample IDs of a study
	QList<int> studySamples(QString study, QStringList& errors);

protected:
	///Copy constructor "declared away".
	GenLabDB(const GenLabDB&) = delete;

	///Returns a SqlQuery object on the GenLab database for custom queries.
	SqlQuery getQuery() const
	{
		return SqlQuery(*db_);
	}

	///Returns the sample and processed sample name
	static QStringList names(QString ps_name);

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;

	static QMap<QString, TableInfo> infos_;
};


#endif // GENLABDB_H
