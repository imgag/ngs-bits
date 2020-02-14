#ifndef GENEOMIMINFOWIDGET_H
#define GENEOMIMINFOWIDGET_H

//TODO remove webservice once it is not used anymore > MARC

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
	void openLink(QString url);
	void copyToClipboard();

private:
	Ui::GeneOmimInfoWidget ui_;

	static QString link(QString number, char prefix);
};

#endif // GENEOMIMINFOWIDGET_H
