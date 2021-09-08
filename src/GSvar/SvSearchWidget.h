#ifndef SVSEARCHWIDGET_H
#define SVSEARCHWIDGET_H

#include <QWidget>
#include "NGSD.h"
#include "ui_SvSearchWidget.h"

class SvSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	SvSearchWidget(QWidget* parent = 0);
	void setVariant(const BedpeLine& sv);
	void setProcessedSampleId(QString ps_id);

protected slots:
	void search();
	void changeSearchType();
	void openSelectedSampleTabs();

private:
	Ui::SvSearchWidget ui_;
	NGSD db_;
	QString ps_id_;
};

#endif // SVSEARCHWIDGET_H
