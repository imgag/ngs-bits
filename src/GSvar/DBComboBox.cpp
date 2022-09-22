#include "DBComboBox.h"
#include "Exceptions.h"


DBComboBox::DBComboBox(QWidget* parent)
	: QComboBox(parent)
{
	setMaxVisibleItems(20);
}

void DBComboBox::fill(const DBTable& table, bool prepend_empty)
{
	if (table.columnCount()!=1)
	{
		THROW(ArgumentException, "Column count is not 1 in DBComboBox::fill: " + QString::number(table.columnCount()));
	}

	if (prepend_empty)
	{
		addItem("", "");
	}

	for (int r=0; r<table.rowCount(); ++r)
	{
		const DBRow& row = table.row(r);
		addItem(row.value(0), row.id());
	}
}

QString DBComboBox::getCurrentId() const
{
	return currentData().toString();
}

void DBComboBox::setCurrentId(QString id)
{
	for (int i=0; i<count(); ++i)
	{
		if (itemData(i).toString()==id)
		{
			setCurrentIndex(i);
			return;
		}
	}

	THROW(ArgumentException, "No entry with ID '" + id + "' found in DBComboBox::setCurrentId!");
}
