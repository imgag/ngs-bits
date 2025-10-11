#include "DBSelector.h"
#include "Exceptions.h"
#include "GUIHelper.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>

DBSelector::DBSelector(QWidget* parent)
	: QLineEdit(parent)
{
	setMinimumWidth(300);
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

	setCompleter(GUIHelper::completer(this, items));
}

bool DBSelector::isValidSelection() const
{
	//empty is not valid
	if(text().isEmpty()) return false;

	return text2id_.contains(text());
}

QString DBSelector::getId() const
{
	if (!text2id_.contains(text()))
	{
		return "";
	}

	return text2id_[text()];
}

void DBSelector::showVisuallyIfValid(bool empty_is_valid)
{
	//check if text is ok
	bool ok = true;
	if (text().isEmpty() && !empty_is_valid) ok = false;
	if (!text().isEmpty() && !text2id_.contains(text())) ok = false;

	//show result
	setStyleSheet(ok ? "" : "QLineEdit {border: 2px solid red;}");
}

void DBSelector::keyPressEvent(QKeyEvent* e)
{
	if (e->matches(QKeySequence::Paste))
	{
		insert(QApplication::clipboard()->text().trimmed());
		e->accept();
		return;
	}

	QLineEdit::keyPressEvent(e);
}
