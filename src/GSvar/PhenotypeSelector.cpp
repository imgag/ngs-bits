#include "PhenotypeSelector.h"
#include "ui_PhenotypeSelector.h"
#include "GUIHelper.h"
#include <QMenu>
#include <QTextEdit>

PhenotypeSelector::PhenotypeSelector(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::PhenotypeSelector)
{
	ui->setupUi(this);
	connect(ui->search, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
	connect(ui->list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(itemDoubleClicked(QListWidgetItem*)));
	connect(ui->list, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(listContextMenu(QPoint)));
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

void PhenotypeSelector::listContextMenu(QPoint p)
{
	QListWidgetItem* item = ui->list->itemAt(p);
	if (item==nullptr) return;

	QMenu menu;
	menu.addAction("Show details");
	QAction* a = menu.exec(ui->list->mapToGlobal(p));
	if(a==nullptr) return;

	//get id/definition
	SqlQuery query = db_.getQuery();
	query.exec("SELECT id, definition FROM hpo_term WHERE name='" + item->text() + "'");
	query.next();
	QString id = query.value(0).toString();
	QString def = query.value(1).toString();

	//get genes
	QStringList genes = db_.getValues("SELECT gene FROM hpo_genes WHERE hpo_term_id=" + id);

	//get parent items
	QStringList parents = db_.getValues("SELECT t.name FROM hpo_term t, hpo_parent p WHERE t.id=p.parent AND p.child=" + id);

	//get child items
	QStringList children = db_.getValues("SELECT t.name FROM hpo_term t, hpo_parent p WHERE t.id=p.child AND p.parent=" + id);

	//show details
	QTextEdit edit;
	edit.setMinimumWidth(300);
	edit.setReadOnly(true);
	edit.setFrameShape(QFrame::NoFrame);
	edit.setText("<b>Definition:</b><br>" + def + "<br><br><b>Parent items:</b><br>" + parents.join(", ") + "<br><br><b>Child items:</b><br>" + children.join(", ") + "<br><br><b>Genes:</b><br>" + genes.join(", "));
	GUIHelper::showWidgetAsDialog(&edit, "Phenotype details", false);
}
