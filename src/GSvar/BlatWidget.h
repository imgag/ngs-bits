#ifndef BLATWIDGET_H
#define BLATWIDGET_H

#include <QWidget>
#include "ui_BlatWidget.h"

class BlatWidget
	: public QWidget
{
	Q_OBJECT

public:
	BlatWidget(QWidget *parent = 0);

private slots:
	void performSearch();
	void resultContextMenu(QPoint pos);

private:
	Ui::BlatWidget ui_;
};

#endif // BLATWIDGET_H
