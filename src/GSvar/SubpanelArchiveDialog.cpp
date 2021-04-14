#include "SubpanelArchiveDialog.h"
#include "Helper.h"
#include "NGSD.h"

SubpanelArchiveDialog::SubpanelArchiveDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, changed_(false)
{
	ui_.setupUi(this);

	connect(ui_.list_subpanel, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(archive(QListWidgetItem*)));
	connect(ui_.list_archive, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(restore(QListWidgetItem*)));
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateSubpanelLists()));

	updateSubpanelLists();
}

bool SubpanelArchiveDialog::changedSubpanels()
{
	return changed_;
}

void SubpanelArchiveDialog::updateSubpanelLists()
{
	//create gene set for filter
	GeneSet genes = GeneSet::createFromText(ui_.f_genes->text().toLatin1().trimmed(), ',');
	QString filename = ui_.f_filename->text().trimmed();

	updateSubpanelList(ui_.list_subpanel, false, genes, filename);
	updateSubpanelList(ui_.list_archive, true, genes, filename);
}

void SubpanelArchiveDialog::updateSubpanelList(QListWidget* list, bool archived, const GeneSet& f_genes, QString f_filename)
{
	NGSD db;

	//get all names
	QStringList names = db.subPanelList(archived);

	//filter
	if (!f_filename.isEmpty())
	{
		Helper::removeIf(names, [f_filename](const QString& name){return !name.contains(f_filename);});
	}

	if (!f_genes.isEmpty())
	{
		QStringList conditions;
		foreach(const QByteArray& gene, f_genes)
		{
			conditions << "genes LIKE '%" + gene + "%'";
		}
		QStringList hits = db.getValues("SELECT name FROM subpanels WHERE " + conditions.join(" AND "));

		Helper::removeIf(names, [hits](const QString& name){return !hits.contains(name);});
	}

	list->clear();
	list->addItems(names);
}

void SubpanelArchiveDialog::archive(QListWidgetItem* item)
{
	if (item==nullptr) return;

	NGSD().getQuery().exec("UPDATE subpanels SET archived=1 WHERE name='" + item->text() + "'");

	updateSubpanelLists();

	changed_ = true;
}

void SubpanelArchiveDialog::restore(QListWidgetItem* item)
{
	if (item==nullptr) return;

	NGSD().getQuery().exec("UPDATE subpanels SET archived=0 WHERE name='" + item->text() + "'");

	updateSubpanelLists();

	changed_ = true;
}
