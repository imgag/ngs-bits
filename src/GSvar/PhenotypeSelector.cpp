#include "PhenotypeSelector.h"
#include "ui_PhenotypeSelector.h"
#include <QDebug>

PhenotypeSelector::PhenotypeSelector(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::PhenotypeSelector)
{
	ui->setupUi(this);
	connect(ui->search, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
	connect(ui->list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemDoubleClicked(QListWidgetItem*)));

	//init search
	search("");
}

PhenotypeSelector::~PhenotypeSelector()
{
	delete ui;
}

void PhenotypeSelector::search(QString text)
{
	//clear
	ui->list->clear();

	//add search terms
	QStringList phenos = db_.phenotypes(text.split(" "));
	foreach(QString pheno, phenos)
	{
		ui->list->addItem(pheno);
	}
}

void PhenotypeSelector::itemDoubleClicked(QListWidgetItem* item)
{
	emit phenotypeSelected(item->text());
}
