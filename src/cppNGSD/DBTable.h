#ifndef DBTABLE_H
#define DBTABLE_H

#include "cppNGSD_global.h"
#include <QString>
#include <QList>

//Database row with identifier and values
class CPPNGSDSHARED_EXPORT DBRow
{
	public:
		DBRow()
		{
		}

		const QString& id() const
		{
			return id_;
		}
		void setId(const QString& id)
		{
			id_ = id;
		}

		int valueCount() const
		{
			return values_.count();
		}
		const QString& value(int i) const;
		void addValue(const QString& value)
		{
			values_.append(value);
		}
		void setValue(int i, const QString& value);
		void removeValue(int i);

	protected:
		QString id_;
		QStringList values_;

		void checkValueIndex(int index) const;
};

//Database table
class CPPNGSDSHARED_EXPORT DBTable
{
	public:
		DBTable()
		{
		}

		QString tableName() const
		{
			return table_name_;
		}
		void setTableName(const QString& table_name)
		{
			table_name_ = table_name;
		}

		int columnCount() const
		{
			return headers_.count();
		}
		const QStringList& headers() const
		{
			return headers_;
		}
		void setHeaders(const QStringList& headers)
		{
			headers_ = headers;
		}

		int rowCount() const
		{
			return rows_.count();
		}
		const DBRow& row(int r) const;
		void setRow(int r, const DBRow& row);
		void addRow(const DBRow& row);
		void reserve(int alloc)
		{
			rows_.reserve(alloc);
		}

		///Returns the index of the column with the given name. Throws an error if not found.
		int columnIndex(const QString& name) const;
		///Adds a new column, returns the index of the new column.
		int addColumn(const QStringList& values, const QString& header);
		///Removes a column and returns the all values.
		QStringList takeColumn(int c);
		///Sets a the values and optionally the header of a column.
		void setColumn(int c, const QStringList& values, const QString& header=QString());
		///Creates and returns a list of values for a column.
		QStringList extractColumn(int c) const;

	protected:
		QString table_name_;
		QStringList headers_;
		QList<DBRow> rows_;

		void checkRowIndex(int r) const;
		void checkColumnIndex(int c) const;
};

#endif // DBTABLE_H
