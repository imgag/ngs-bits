#include "DBQCWidget.h"

#include <QDebug>

DBQCWidget::DBQCWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
}

void DBQCWidget::setSystemId(QString id)
{
	id_sys_ = id;

	updateGUI();
}

void DBQCWidget::updateGUI()
{
	if (id_term_.isEmpty()) return;

	//create query
	QString query_string = "SELECT qc.value, ps.quality, r.end_date FROM processed_sample_qc qc, processed_sample ps, sequencing_run r WHERE qc.processed_sample_id=ps.id AND ps.sequencing_run_id=r.id AND qc.qc_terms_id='" + id_term_ + "'";
	if (!id_sys_.isEmpty())
	{
		query_string += " AND ps.processing_system_id='" + id_sys_ + "'";
	}

	//execute query
	SqlQuery query = db_.getQuery();
	query.exec(query_string);
	while(query.next())
	{
		qDebug() << query.value(0) << query.value(1) << query.value(2);
	}
}

void DBQCWidget::setTermId(QString id)
{
	id_term_ = id;

	updateGUI();
}
