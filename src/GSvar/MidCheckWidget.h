#ifndef MIDCHECKWIDGET_H
#define MIDCHECKWIDGET_H

#include "ui_MidCheckWidget.h"
#include <QWidget>
#include "MidCheck.h"

class MidCheckWidget
	: public QWidget
{
	Q_OBJECT

public:
	MidCheckWidget(QWidget *parent = 0);
	void setParameters(QPair<int,int> lengths);
	void setMids(const QList<SampleMids>& mids);

protected slots:
	void updateSampleTable();

private:
	Ui::MidCheckWidget ui_;
	QList<SampleMids> mids_;


	QTableWidgetItem* createItem(const QString& text, int alignment = Qt::AlignVCenter|Qt::AlignLeft);
};

#endif // MIDCHECKWIDGET_H
