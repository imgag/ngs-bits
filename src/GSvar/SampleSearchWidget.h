#ifndef SAMPLESEARCHWIDGET_H
#define SAMPLESEARCHWIDGET_H

#include <QWidget>
#include "ui_SampleSearchWidget.h"
#include "NGSD.h"

class SampleSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	SampleSearchWidget(QWidget *parent = 0);

protected:

protected slots:
	void checkValid();
	void search();

private:
	Ui::SampleSearchWidget ui_;
	NGSD db_;
	bool is_valid_;
};

#endif // SAMPLESEARCHWIDGET_H
