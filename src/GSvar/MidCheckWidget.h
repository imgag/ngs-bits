#ifndef MIDCHECKWIDGET_H
#define MIDCHECKWIDGET_H

#include "ui_MidCheckWidget.h"
#include <QWidget>
#include "MidCheck.h"
#include "NGSD.h"

class MidCheckWidget
	: public QWidget
{
	Q_OBJECT

public:
	MidCheckWidget(QWidget *parent = 0);
	const QList<SampleMids> mids() const;

	void setParameters(QPair<int,int> lengths);
	void addRun(QString run_name);

protected slots:
	void updateSampleTable();
	void checkMids();
	void add();
	void addBatch();
	void addRun();

private:
	Ui::MidCheckWidget ui_;
	QList<SampleMids> mids_;
	NGSD db_;

	SampleMids parseImportLine(QString line, int line_nr = 1);
};

#endif // MIDCHECKWIDGET_H
