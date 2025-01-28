#include "DBTable.h"
#include "Exceptions.h"

const DBRow& DBTable::row(int r) const
{
	checkRowIndex(r);

	return rows_[r];
}

void DBTable::setRow(int r, const DBRow& row)
{
	checkRowIndex(r);

	rows_[r] = row;
}

void DBTable::setValue(int r, int c, const QString& value)
{
	checkRowIndex(r);

	return rows_[r].setValue(c, value);
}

void DBTable::addRow(const DBRow& row)
{
	if (row.valueCount()!=columnCount())
	{
		THROW(ArgumentException, "Cannot add row with '" + QString::number(row.valueCount()) + "' elements to DB table '" + table_name_ + "'. Expected '" + QString::number(columnCount()) + "' values!");
	}

	rows_.append(row);
}

void DBTable::removeRow(int r)
{
	checkRowIndex(r);

	rows_.removeAt(r);
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

void DBTable::insertColumn(int i, const QStringList& values, const QString& header)
{
	//check
	if (values.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid value count '" + QString::number(values.count()) + "' in DB table for '" + table_name_ + "'. Expected " + QString::number(rowCount()) + "!");
	}

	//header
	headers_.insert(i, header);

	//content
	for (int r=0; r<rowCount(); ++r)
	{
		rows_[r].insertValue(i, values[r]);
	}
}

int DBTable::addColumn(const QStringList& values, const QString& header)
{
	//check
	if (values.count()!=rowCount())
	{
		THROW(ArgumentException, "Invalid value count '" + QString::number(values.count()) + "' in DB table for '" + table_name_ + "'. Expected " + QString::number(rowCount()) + "!");
	}

	//header
	headers_ << header;

	//content
	for (int r=0; r<rowCount(); ++r)
	{
		rows_[r].addValue(values[r]);
	}

	return columnCount() - 1;
}

QStringList DBTable::takeColumn(int c)
{
	checkColumnIndex(c);

	//headers
	headers_.removeAt(c);

	//content
	QStringList output;
	output.reserve(rowCount());
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
	output.reserve(rowCount());
	for (int r=0; r<rowCount(); ++r)
	{
		output << rows_[r].value(c);
	}
	return output;
}

bool DBTable::columnIsNumeric(int c) const
{
	//check
	checkColumnIndex(c);

	//content
	for (int r=0; r<rowCount(); ++r)
	{
		//check if convertable to number
		bool ok = false;
		rows_[r].value(c).toDouble(&ok);
		if (!ok) return false;
	}

	return true;
}

void DBTable::formatBooleanColumn(int c, bool empty_if_no)
{
	//init
	static QString s_yes = "yes";
	static QString s_no = "no";

	//check
	checkColumnIndex(c);


	//content
	for (int r=0; r<rowCount(); ++r)
	{
		const QString& value = rows_[r].value(c);
		if (value=="1")
		{
			rows_[r].setValue(c, s_yes);

		}
		else if (value=="0")
		{
			rows_[r].setValue(c, empty_if_no ? "" : s_no);
		}
		else if (!value.isEmpty())
		{
			THROW(ProgrammingException, "Unhandled value '" + value + "' in DBTable::formatBooleanColumn!");
		}
	}
}

void DBTable::filterRows(QString text, Qt::CaseSensitivity cs)
{
	for(int r=rowCount()-1; r>=0; --r) //reverse, so that all indices are valid
	{
		if (!rows_[r].contains(text, cs))
		{
			rows_.removeAt(r);
		}
	}

}

void DBTable::filterRowsByColumn(int col_idx, QString text, Qt::CaseSensitivity cs)
{
	for(int r=rowCount()-1; r>=0; --r) //reverse, so that all indices are valid
	{
		if (!rows_[r].value(col_idx).contains(text, cs))
		{
			rows_.removeAt(r);
		}
	}
}

void DBTable::filterRowsByColumn(int col_idx, QStringList texts, Qt::CaseSensitivity cs)
{
	for(int r=rowCount()-1; r>=0; --r) //reverse, so that all indices are valid
	{
		bool contained = false;
		foreach(const QString& text, texts)
		{
			if (rows_[r].value(col_idx).contains(text, cs))
			{
				contained = true;
				break;
			}
		}
		if (!contained)
		{
			rows_.removeAt(r);
		}
	}
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

bool DBRow::contains(const QString& text, Qt::CaseSensitivity cs) const
{
	foreach(const QString& value, values_)
	{
		if (value.contains(text, cs)) return true;
	}

	return false;
}

void DBRow::checkValueIndex(int i) const
{
	if (i<0 || i>=values_.count())
	{
		THROW(ArgumentException, "Invalid value index '" + QString::number(i) + "' in DB row. Valid are 0-" + QString::number(values_.count()-1) + "!");
	}
}

void DBTable::write(QTextStream& stream) const
{
	//header
	stream << "#";
	for (int c=0; c<headers_.count(); ++c)
	{
		if (c!=0) stream << "\t";
		stream << headers_[c];
	}
	stream << "\n";

	//rows
	for (int r=0; r<rowCount(); ++r)
	{
		for (int c=0; c<columnCount(); ++c)
		{
			if (c!=0) stream << "\t";
			stream << QString(rows_[r].value(c)).replace('\t', ' ').replace('\n', ' ').replace('\r', ' ');
		}
		stream << "\n";
	}
}

