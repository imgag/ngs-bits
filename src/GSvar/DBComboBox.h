#ifndef DBCOMBOBOX_H
#define DBCOMBOBOX_H

#include "DBTable.h"

#include <QComboBox>

///Selector for database entries (using DB ID).
class DBComboBox
	: public QComboBox
{
	Q_OBJECT

public:
	DBComboBox(QWidget* parent = 0);

	///Fills the list from a database table
	void fill(const DBTable table, bool prepend_empty = true);

	///Returns the current database ID as string (or an empty string if no entry is selected)
	QString getCurrentId() const;
	///Sets the current database ID.
	void setCurrentId(QString id);

};

#endif // DBCOMBOBOX_H
