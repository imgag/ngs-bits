#include "GBDOFKEdit.h"
#include "DatabaseCache.h"
#include <QStringListModel>

GBDOFKEdit::GBDOFKEdit(QString table, QString field, QWidget* parent)
	: QLineEdit(parent)
	, table_(table)
	, field_(field)
	, id_(-1)
{
	QCompleter* completer = new QCompleter(QStringList(), this);
	completer->setFilterMode(Qt::MatchContains);
	setCompleter(completer);

	connect(this, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
	setStyleSheet("QLineEdit {background: #BFDDFF;};");
}

void GBDOFKEdit::setId(int id)
{
	QString text = DatabaseCache::inst().ngsd().getValue("SELECT " + field_ + " FROM " + table_ + " WHERE id=" + QString::number(id)).toString();
	setText(text);
	id_ = id;
}

int GBDOFKEdit::id()
{
	return id_;
}

void GBDOFKEdit::search(QString text)
{
	//reset
	id_ = -1;
	if (text=="") return;

	//set ID if we find a perfect match
	SqlQuery query = DatabaseCache::inst().ngsd().getQuery();
	query.prepare("SELECT id FROM " + table_ + " WHERE " + field_ + "=:text");
	query.bindValue(":text", text);
	query.exec();
	if (query.size()==1)
	{
		query.next();
		id_ = query.value(0).toInt();
		return;
	}

	//create autocompletion list otherwise
	query.prepare("SELECT " + field_ + " FROM " + table_ + " WHERE " + field_ + " LIKE :text LIMIT 20");
	query.bindValue(":text", "%"+text+"%");
	query.exec();
	QStringList items;
	while (query.next())
	{
		items << query.value(0).toString();
	}
	QStringListModel* model = qobject_cast<QStringListModel*>(completer()->model());
	model->setStringList(items);
}

