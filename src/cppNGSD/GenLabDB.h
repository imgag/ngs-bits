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
	///Returns if the database connection is open
	bool isOpen() const;

	///Returns the phenotypes of an NGSD sample (not processed sample)
	QList<Phenotype> phenotypes(QString sample_name);

	///Returns the ICD10 diagnosis of an NGSD sample (not processed sample)
	QString diagnosis(QString sample_name);

	///Returns tumor content of an NGSD sample (not processed sample)
	QString tumorFraction(QString sample_name);


protected:
	///Copy constructor "declared away".
	GenLabDB(const GenLabDB&) = delete;

	///The database adapter
	QSharedPointer<QSqlDatabase> db_;
	bool is_open_;
};


#endif // GENLABDB_H
