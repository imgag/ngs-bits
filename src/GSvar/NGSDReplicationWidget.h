#ifndef NGSDREPLICATIONWIDGET_H
#define NGSDREPLICATIONWIDGET_H

#include <QWidget>
#include "ui_NGSDReplicationWidget.h"
#include "NGSD.h"
#include "FastaFileIndex.h"

class NGSDReplicationWidget
	: public QWidget
{
	Q_OBJECT

public:
	NGSDReplicationWidget(QWidget* parent = 0);

protected slots:
	void replicate();

protected:
	void addLine(QString text);
	void addHeader(QString text);
	void addWarning(QString text);
	void addError(QString text);
	void performPreChecks();
	void replicateBaseData();
	void replicateBaseDataNoId();
	void addSampleGroups();
	void replicateVariantData();
	void performPostChecks();

	int liftOverVariant(int source_variant_id);
	void updateTable(QString table, bool contains_variant_id=false, QString sample_id="");

private:
	Ui::NGSDReplicationWidget ui_;
	QSharedPointer<NGSD> db_source_;
	QSharedPointer<NGSD> db_target_;
	QSharedPointer<FastaFileIndex> genome_index_;
	QSharedPointer<FastaFileIndex> genome_index_hg38_;

	QSet<QString> tables_done_;

};

#endif // NGSDREPLICATIONWIDGET_H
