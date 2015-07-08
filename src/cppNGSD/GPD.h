#ifndef GPD_H
#define GPD_H

#include "cppNGSD_global.h"
#include <QSqlDatabase>
#include <QVariant>
#include "VariantList.h"

/// GPD accessor.
class CPPNGSDSHARED_EXPORT GPD
{
public:
	///Default constructor that connects to the DB.
	GPD();
	///Destructor.
	~GPD();

	///Connects to the database and throws a DatabaseError if the connection fails.
	bool connect();

	///Annotates (or re-annotates) the variant list with current NGDS information.
	void annotate(VariantList& variants);

	///Executes an SQL query and returns the single return value.
	///If no values are returned an error thrown or a default-constructed QVariant is returned (depending on @p empty_is_ok).
	///If more than one value is returned a DatabaseError is thrown.
	QVariant getValue(const QString& query, bool no_value_is_ok=true);
	///Executes an SQL query and returns it.
	QSqlQuery execute(const QString& query);

protected:
	///Copy constructor "declared away".
	GPD(const GPD&);

	///Adds a sample-specific string column with empty entries. Returns the index of the added column.
	int addColumn(VariantList& variants, QString name, QString description);
	///Removes a column if it is present. Returns if the column was present.
	bool removeColumnIfPresent(VariantList& variants, QString name, bool exact_name_match);

	///The database adapter
	QSqlDatabase db_;
};

#endif // GPD_H
