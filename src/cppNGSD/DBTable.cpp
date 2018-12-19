#include "DBTable.h"
#include "Exceptions.h"

#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>

const DBRow&DBTable::row(int r) const
{
	checkRowIndex(r);

	return rows_[r];
}

void DBTable::setRow(int r, const DBRow& row)
{
	checkRowIndex(r);

	rows_[r] = row;
}

void DBTable::addRow(const DBRow& row)
{
	if (row.valueCount()!=columnCount())
	{
		THROW(ArgumentException, "Cannot add row with '" + QString::number(row.valueCount()) + "' elements to DB table '" + table_name_ + "'. Expected '" + QString::number(columnCount()) + "' values!");
	}

	rows_.append(row);
}

int DBTable::columnIndex(const QString& name) const
{
	QList<int> output;

	for (int i=0; i<headers_.count(); ++i)
	{
		if (headers_[i]==name)
		{
			output << i;
		}
	}

	if (output.count()==0)
	{
		THROW(ArgumentException, "Colum with name '" + name + "' not found in DB table '" + table_name_ + "'. Valid names are: " + headers_.join(", "));
	}
	if (output.count()>1)
	{
		THROW(ArgumentException, "Colum with name '" + name + "' found several times in DB table '" + table_name_ + "'!");
	}

	return output[0];
}

QStringList DBTable::takeColumn(int c)
{
	checkColumnIndex(c);

	//headers
	headers_.removeAt(c);

	//content
	QStringList output;
	for (int r=0; r<rows_.count(); ++r)
	{
		DBRow current_row = row(r);
		output << current_row.value(c);
		current_row.removeValue(c);
		setRow(r, current_row);
	}

	return output;
}

void DBTable::setColumn(int c, const QStringList& values, const QString& header)
{
	//check
	checkColumnIndex(c);
	if (values.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid value count '" + QString::number(values.count()) + "' in DB table for '" + table_name_ + "'. Expected " + QString::number(rowCount()) + "!");
	}

	//header
	if (!header.isEmpty())
	{
		headers_[c] = header;
	}

	//content
	for (int r=0; r<rowCount(); ++r)
	{
		rows_[r].setValue(c, values[r]);
	}
}

QStringList DBTable::extractColumn(int c) const
{
	//check
	checkColumnIndex(c);

	//content
	QStringList output;
	for (int r=0; r<rowCount(); ++r)
	{
		output << rows_[r].value(c);
	}
	return output;
}

void DBTable::checkRowIndex(int r) const
{
	if (r<0 || r>=rows_.count())
	{
		THROW(ArgumentException, "Invalid row index '" + QString::number(r) + "' in DB table for '" + table_name_ + "'. Valid are 0-" + QString::number(rows_.count()-1) + "!");
	}
}

void DBTable::checkColumnIndex(int c) const
{
	if (c<0 || c>=columnCount())
	{
		THROW(ArgumentException, "Invalid column index '" + QString::number(c) + "' in DB table for '" + table_name_ + "'. Valid are 0-" + QString::number(columnCount()-1) + "!");
	}
}

const QString& DBRow::value(int i) const
{
	checkValueIndex(i);

	return values_[i];
}

void DBRow::setValue(int i, const QString& value)
{
	checkValueIndex(i);

	values_[i] = value;
}

void DBRow::removeValue(int i)
{
	checkValueIndex(i);

	values_.removeAt(i);
}

void DBRow::checkValueIndex(int i) const
{
	if (i<0 || i>=values_.count())
	{
		THROW(ArgumentException, "Invalid value index '" + QString::number(i) + "' in DB row. Valid are 0-" + QString::number(values_.count()-1) + "!");
	}
}
