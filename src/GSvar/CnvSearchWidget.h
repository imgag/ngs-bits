#ifndef CNVSEARCHWIDGET_H
#define CNVSEARCHWIDGET_H

#include <QWidget>
#include "NGSD.h"
#include "ui_CnvSearchWidget.h"

class CnvSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	CnvSearchWidget(QWidget* parent = 0);

protected slots:
	void search();

private:
	Ui::CnvSearchWidget ui_;
	NGSD db_;
};

#endif // CNVSEARCHWIDGET_H
