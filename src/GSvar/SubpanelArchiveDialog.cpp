#include "SubpanelArchiveDialog.h"
#include "Helper.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include <QMenu>
#include <QTextEdit>
#include <QMessageBox>

SubpanelArchiveDialog::SubpanelArchiveDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, changed_(false)
{
	ui_.setupUi(this);

	connect(ui_.list_subpanel, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(archive(QListWidgetItem*)));
	connect(ui_.list_subpanel, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(activePanelContextMenu(QPoint)));
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
	GeneSet genes = GeneSet::createFromText(ui_.f_genes->text().toUtf8().trimmed(), ',');
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
		Helper::removeIf(names, [f_filename](const QString& name){return !name.contains(f_filename, Qt::CaseInsensitive);});
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

void SubpanelArchiveDialog::activePanelContextMenu(QPoint pos)
{
	//extract selected rows
	QList<QListWidgetItem*> items = ui_.list_subpanel->selectedItems();
	if (items.isEmpty()) return;

	//execute menu
	QMenu menu;
	QAction* a_edit_roi = menu.addAction(QIcon(), "Edit target region manually");
	QAction* action = menu.exec(ui_.list_subpanel->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	if  (action==a_edit_roi)
	{
		QString name = items[0]->text();
		QString title = "Edit target region of sub-panel '" + name + "'";

		NGSD db;
		QString roi = db.getValue("SELECT roi FROM subpanels WHERE name='"+name+"'").toString();

		QTextEdit* edit = new QTextEdit(this);
		edit->setAcceptRichText(false);
		edit->setMinimumSize(500, 700);
		edit->setWordWrapMode(QTextOption::NoWrap);
		edit->setText(roi);
		auto dlg = GUIHelper::createDialog(edit, title , "", true);
		if (dlg->exec()==QDialog::Accepted)
		{
			try
			{
				//sort/merge new BED file (also throws an exception during parsing if it is not valid)
				BedFile roi_new = BedFile::fromText(edit->toPlainText().toUtf8());
				roi_new.merge();

				//store
				SqlQuery query = db.getQuery();
				query.prepare("UPDATE subpanels SET roi=:0 WHERE name=:1");
				query.bindValue(0, roi_new.toText());
				query.bindValue(1, name);
				query.exec();

				//show dialog with infos
				BedFile roi_old = BedFile::fromText(roi.toUtf8());
				int delta = roi_new.baseCount() - roi_old.baseCount();
				QMessageBox::information(this, title, "Stored sub-panel target region in NGSD.\n"
													  "Before the update it contained " + QString::number(roi_old.baseCount()) + " bases, now it contains " + QString::number(roi_new.baseCount()) + " bases (delta: "+(delta>=0 ? "+" : "")+QString::number(delta)+" bases).");

			}
			catch(Exception& e)
			{
				QMessageBox::warning(this, title, "Could not store target region in NGSD:\n"+e.message());
			}
		}
	}
}
