#include "ProcessedSampleFKEdit.h"
#include "DatabaseCache.h"
#include <QStringListModel>

ProcessedSampleFKEdit::ProcessedSampleFKEdit(QWidget* parent)
	: GBDOFKEdit("processed_sample", "process_id", parent)
{
	connect(this, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
}

void ProcessedSampleFKEdit::search(QString text)
{
	//reset
	id_ = -1;
	if (text=="") return;

	QStringList parts = text.split("_");
	QString sample = parts.count()>0 ? parts[0] : "";
	QString psnum = parts.count()>1 ? parts[1] : "";

	//set ID if we find a perfect match
	SqlQuery query = DatabaseCache::inst().ngsd().getQuery();
	query.prepare("SELECT ps.id FROM processed_sample ps, sample s WHERE s.name=:sample AND ps.sample_id=s.id AND ps.process_id=:psnum");
	query.bindValue(":sample", sample);
	query.bindValue(":psnum", psnum);
	query.exec();
	if (query.size()==1)
	{
		query.next();
		id_ = query.value(0).toInt();
		return;
	}

	//create autocompletion list otherwise
	query.prepare("SELECT s.name, ps.process_id FROM processed_sample ps, sample s WHERE s.name LIKE :sample AND ps.sample_id=s.id AND ps.process_id LIKE :psnum LIMIT 20");
	query.bindValue(":sample", "%"+sample+"%");
	query.bindValue(":psnum", "%"+psnum+"%");
	query.exec();
	QStringList items;
	while (query.next())
	{
		items << query.value(0).toString() + "_" + query.value(1).toString().rightJustified(2 , '0');
	}
	QStringListModel* model = qobject_cast<QStringListModel*>(completer()->model());
	model->setStringList(items);
}

