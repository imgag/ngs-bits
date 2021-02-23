#ifndef PRSVIEW_H
#define PRSVIEW_H

#include "ui_PRSWidget.h"
#include "PrsTable.h"

class PRSWidget
	: public QWidget
{
	Q_OBJECT

public:
	PRSWidget(QString filename, QWidget *parent = 0);

protected slots:
	void showContextMenu(QPoint pos);

private:
	void initGui();

	Ui::PRSView ui_;
	PrsTable prs_table_;
};

#endif // PRSVIEW_H
