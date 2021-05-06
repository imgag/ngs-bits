#include "PhenotypeSelector.h"
#include "ui_PhenotypeSelector.h"
#include "GUIHelper.h"
#include <QMenu>
#include <QTextEdit>

PhenotypeSelector::PhenotypeSelector(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::PhenotypeSelector)
	, details_(nullptr)
{
	ui->setupUi(this);
	connect(ui->search, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
	connect(ui->list, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(itemActivated(QListWidgetItem*)));
	connect(ui->list, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(itemChanged(QListWidgetItem*)));
}

PhenotypeSelector::~PhenotypeSelector()
{
	delete ui;
}

void PhenotypeSelector::setDetailsWidget(QTextEdit* edit)
{
	details_ = edit;
	details_->setReadOnly(true);
	details_->setPlaceholderText("phenotype details");
}

void PhenotypeSelector::init()
{
	search("");
}

void PhenotypeSelector::search(QString text)
{
	//clear
	ui->list->clear();

	//no search without string (too many entries)
	text = text.trimmed();
	if (text.isEmpty()) return;

	//add search terms
	PhenotypeList phenos = db_.phenotypes(text.split(" "));
	foreach(const Phenotype& pheno, phenos)
	{
		ui->list->addItem(pheno.name());
	}
}

const Phenotype& PhenotypeSelector::nameToPhenotype(QByteArray name) const
{
	return db_.phenotype(db_.phenotypeIdByName(name));
}

void PhenotypeSelector::keyPressEvent(QKeyEvent* event)
{
	if (event->key()==Qt::Key_Return)
	{
		QString term_name = ui->search->text().trimmed();
		for (int i=0; i<ui->list->count(); ++i)
		{
			QListWidgetItem* item = ui->list->item(i);
			if (item!=nullptr && item->text().compare(term_name, Qt::CaseInsensitive)==0)
			{
				emit phenotypeActivated(item->text());
				return;
			}
		}
	}
	else
	{
		QWidget::keyPressEvent(event);
	}
}

void PhenotypeSelector::itemChanged(QListWidgetItem* item)
{
	//update details
	if (details_!=nullptr)
	{
		details_->setText(selectedItemDetails(true, false));
	}

	//special handling for not item
	if (item==nullptr)
	{
		emit phenotypeChanged("");
		return;
	}

	emit phenotypeChanged(item->text());
}

void PhenotypeSelector::itemActivated(QListWidgetItem* item)
{
	if (item==nullptr) return;

	emit phenotypeActivated(item->text());
}

QString PhenotypeSelector::selectedItemDetails(bool show_name, bool shown_genes)
{
	QListWidgetItem* item = ui->list->currentItem();
	if (item==nullptr) return "";

	//get id/definition
	SqlQuery query = db_.getQuery();
	query.prepare("SELECT id, hpo_id, name, definition, synonyms FROM hpo_term WHERE name=:0");
	query.bindValue(0, item->text());
	query.exec();
	query.next();
	QString id = query.value(0).toString();
	QString output;
	if (show_name)
	{
		output = "<b>" + query.value(2).toString() + " (" + query.value(1).toString() +")</b><br><br>";
	}
	output += "<b>Definition:</b><br>" + query.value(3).toString();

	output += "<br><br><b>Synonyms:</b><br>" + query.value(4).toString().replace("\n", ", ");

	//get parent items
	QStringList parents = db_.getValues("SELECT t.name FROM hpo_term t, hpo_parent p WHERE t.id=p.parent AND p.child=" + id);
	output += "<br><br><b>Parent items:</b><br>" + parents.join(", ");

	//get child items
	QStringList children = db_.getValues("SELECT t.name FROM hpo_term t, hpo_parent p WHERE t.id=p.child AND p.parent=" + id);
	output += "<br><br><b>Child items:</b><br>" + children.join(", ");

	//get genes
	if (shown_genes)
	{
		QStringList genes = db_.getValues("SELECT gene FROM hpo_genes WHERE hpo_term_id=" + id);
		output += "<br><br><b>Genes:</b><br>" + genes.join(", ");
	}

	return	output;
}
