#ifndef NGSDREPLICATIONWIDGET_H
#define NGSDREPLICATIONWIDGET_H

#include <QWidget>
#include "ui_NGSDReplicationWidget.h"
#include "NGSD.h"

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
	void performPreChecks(NGSD& source, NGSD& target);
	void replicateBaseData(NGSD& source, NGSD& target);
	void replicateVariantData(NGSD& source, NGSD& target);
	void performPostChecks(NGSD& source, NGSD& target);

private:
	Ui::NGSDReplicationWidget ui_;
	QSet<QString> tables_done_;

};

#endif // NGSDREPLICATIONWIDGET_H
