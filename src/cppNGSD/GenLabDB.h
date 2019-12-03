#ifndef GENLABDB_H
#define GENLABDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSharedPointer>
#include "cppNGSD_global.h"
#include <Phenotype.h>

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
	QStringList tables();

	///Returns the number of entries with the given sample name
	bool entriesExistForSample(QString sample_name);

	///Returns HPO phenotypes of a sample (try processed sample if not found - this is not consistent in GenLab)
	QList<Phenotype> phenotypes(QString sample_name);

	///Returns Oprhanet identifiers of a sample (try processed sample if not found - this is not consistent in GenLab)
	QStringList orphanet(QString sample_name);

	///Returns the ICD10 diagnosis of a sample (try processed sample if not found - this is not consistent in GenLab)
	QStringList diagnosis(QString sample_name);

	///Returns tumor content of a sample (try processed sample if not found - this is not consistent in GenLab)
	QStringList tumorFraction(QString sample_name);

	///Returns disease group and disease status of a processed sample
	QPair<QString, QString> diseaseInfo(QString ps_name);

protected:
	///Copy constructor "declared away".
	GenLabDB(const GenLabDB&) = delete;

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool is_open_;
};


#endif // GENLABDB_H
