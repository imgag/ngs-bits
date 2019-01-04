#ifndef DBSELECTOR_H
#define DBSELECTOR_H


#include "DBTable.h"

#include <QLineEdit>

///Autocompletion-style selector for database entries (using DB ID).
class DBSelector
	: public QLineEdit
{
	Q_OBJECT

public:
	DBSelector(QWidget* parent);

	///Fills the list from a database table
	void fill(const DBTable& table, bool prepend_empty = true);

	///Returns if a non-empty and valid entry was selected
	bool isValidSelection() const;

	///Returns the database ID as a string (or "" if no entry is selected)
	QString getId() const;

protected:
	QHash<QString, QString> text2id_;
};

#endif // DBSELECTOR_H
