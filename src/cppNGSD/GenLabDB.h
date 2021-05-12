#ifndef GENLABDB_H
#define GENLABDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSharedPointer>
#include "cppNGSD_global.h"
#include "PhenotypeList.h"
#include "NGSD.h"

/// GenLabDB accessor
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

	///Returns all tables in the database
	QStringList tables() const;
	///Returns table meta data.
	const TableInfo& tableInfo(const QString& table) const;

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

	///Returns the sample gender
	QString gender(QString ps_name);

	///Imports missing sample meta data (disease group/status/details) for a sample into NGSD
	void addMissingMetaDataToNGSD(QString ps_name, bool log=false, bool add_disease_group_status=true, bool add_disease_details=true, bool add_gender=true, bool add_relations=true);

protected:
	///Copy constructor "declared away".
	GenLabDB(const GenLabDB&) = delete;

	///Returns a SqlQuery object on the GenLab database for custom queries.
	SqlQuery getQuery() const
	{
		return SqlQuery(*db_);
	}

	///Adds a disease info item to the list, if it is missing. Returns if an item was added.
	static bool addDiseaseInfoIfMissing(QString type, QString value, QDateTime date, QString user, QList<SampleDiseaseInfo>& disease_details);

	///Returns the sample and processed sample name
	static QStringList names(QString ps_name);

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool is_open_;

	static QMap<QString, TableInfo> infos_;
};


#endif // GENLABDB_H
