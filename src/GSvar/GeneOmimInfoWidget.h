#ifndef GENEOMIMINFOWIDGET_H
#define GENEOMIMINFOWIDGET_H

#include <QWidget>
#include "ui_GeneOmimInfoWidget.h"

class GeneOmimInfoWidget
	: public QWidget
{
	Q_OBJECT

public:
	GeneOmimInfoWidget(QWidget *parent = 0);

protected slots:
	void updateTable();
	void copyToClipboard();

private:
	Ui::GeneOmimInfoWidget ui_;

	static QString link(QString number, char prefix);
};

#endif // GENEOMIMINFOWIDGET_H
