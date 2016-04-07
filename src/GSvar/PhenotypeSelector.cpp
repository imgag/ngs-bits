#include "PhenotypeSelector.h"
#include "ui_PhenotypeSelector.h"
#include <QDebug>

PhenotypeSelector::PhenotypeSelector(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PhenotypeSelector)
{
	ui->setupUi(this);
	connect(ui->search, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
}

PhenotypeSelector::~PhenotypeSelector()
{
	delete ui;
}

void PhenotypeSelector::search(QString text)
{
	qDebug() << "SEARCH: " << text;
	QStringList phenos = db_.phenotypes(text.split(" "));
	qDebug() << "PHENOS: " << phenos;
	foreach(QString pheno, phenos)
	{
		ui->list->addItem(pheno);
	}
}
