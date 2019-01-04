#include "DBSelector.h"
#include "Exceptions.h"
#include <QCompleter>
#include <QStringListModel>

DBSelector::DBSelector(QWidget* parent)
	: QLineEdit(parent)
{
}

void DBSelector::fill(const DBTable& table, bool prepend_empty)
{
	if (table.columnCount()!=1)
	{
		THROW(ArgumentException, "Column count is not 1 in DBSelector::fill: " + QString::number(table.columnCount()));
	}

	QStringList items;
	if (prepend_empty)
	{
		items << "";
		text2id_[""] = "";
	}
	for (int r=0; r<table.rowCount(); ++r)
	{
		const DBRow& row = table.row(r);
		items << row.value(0);
		text2id_[row.value(0)] = row.id();
	}

	QCompleter* completer = new QCompleter(items, this);
	completer->setFilterMode(Qt::MatchContains);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setCompletionRole(Qt::DisplayRole);
	completer->setCompletionMode(QCompleter::PopupCompletion);
	setCompleter(completer);
}

bool DBSelector::isValidSelection() const
{
	//empty is not valid
	if(text().isEmpty()) return false;

	return text2id_.contains(text());
}

QString DBSelector::getId() const
{
	return text2id_[text()];
}
