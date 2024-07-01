#ifndef RESEARCHWIDGET_H
#define RESEARCHWIDGET_H

#include <QWidget>
#include "ui_ReSearchWidget.h"
#include "NGSD.h"


class ReSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	ReSearchWidget(QWidget* parent = nullptr);

protected slots:
	void search();
	void showRepeatImage();

private:
	Ui::ReSearchWidget ui_;
	NGSD db_;
};

#endif // RESEARCHWIDGET_H
