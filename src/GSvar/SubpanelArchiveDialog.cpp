#include "SubpanelArchiveDialog.h"
#include "ui_SubpanelArchiveDialog.h"
#include "Settings.h"
#include "Helper.h"
#include "NGSD.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>

SubpanelArchiveDialog::SubpanelArchiveDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::SubpanelArchiveDialog)
	, changed(false)
{
	ui->setupUi(this);

	connect(ui->open_folder, SIGNAL(clicked(bool)), this, SLOT(openSubpanelFolder()));
	connect(ui->list_subpanel, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(archive(QListWidgetItem*)));
	connect(ui->list_archive, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(restore(QListWidgetItem*)));
	connect(ui->update_btn, SIGNAL(clicked(bool)), this, SLOT(updateSubpanelLists()));

	path_subpanel = Helper::canonicalPath(NGSD::getTargetFilePath(true));
	path_archive = Helper::canonicalPath(path_subpanel + "/archive/");
	updateSubpanelLists();
}

SubpanelArchiveDialog::~SubpanelArchiveDialog()
{
	delete ui;
}

bool SubpanelArchiveDialog::changedSubpanels()
{
	return changed;
}

void SubpanelArchiveDialog::openSubpanelFolder()
{
	QString path = NGSD::getTargetFilePath(true);
	QString url = "file:///" + Helper::canonicalPath(path);
	QDesktopServices::openUrl(url);
}

void SubpanelArchiveDialog::updateSubpanelLists()
{
	//create gene set for filter
	GeneSet genes = GeneSet::createFromText(ui->f_genes->text().toLatin1().trimmed(), ',');
	QString filename = ui->f_filename->text().trimmed();

	updateSubpanelList(ui->list_subpanel, path_subpanel, genes, filename);
	updateSubpanelList(ui->list_archive, path_archive, genes, filename);
}

void SubpanelArchiveDialog::updateSubpanelList(QListWidget* list, QString path, const GeneSet& f_genes, QString f_filename)
{
	list->clear();

	QStringList files = Helper::findFiles(path, "*.bed", false);
	foreach(QString file, files)
	{
		if (file.endsWith("_amplicons.bed")) continue;

		//filter by filename
		if (!f_filename.isEmpty() && !file.contains(f_filename))  continue;

		//filter by genes
		if (!f_genes.isEmpty())
		{
			//no gene file => skip
			QString genes_file = QString(file).replace(".bed", "_genes.txt");
			if (!QFile::exists(genes_file)) continue;

			//not all genes contained => skip
			if (!GeneSet::createFromFile(genes_file).containsAll(f_genes)) continue;
		}

		QString name = QFileInfo(file).fileName().replace(".bed", "");
		list->addItem(name);
	}
}

void SubpanelArchiveDialog::archive(QListWidgetItem* item)
{
	if (item==nullptr) return;

	move(item->text() + ".bed", path_subpanel, path_archive);
	move(item->text() + "_genes.txt", path_subpanel, path_archive);

	QString amplicons = item->text() + "_amplicons.bed";
	if (QFile::exists(amplicons))
	{
		move(item->text() + "_amplicons.bed", path_subpanel, path_archive);
	}

	updateSubpanelLists();
}

void SubpanelArchiveDialog::restore(QListWidgetItem* item)
{
	if (item==nullptr) return;

	move(item->text() + ".bed", path_archive, path_subpanel);
	move(item->text() + "_genes.txt", path_archive, path_subpanel);

	QString amplicons = item->text() + "_amplicons.bed";
	if (QFile::exists(amplicons))
	{
		move(item->text() + "_amplicons.bed", path_archive, path_subpanel);
	}

	updateSubpanelLists();
}

void SubpanelArchiveDialog::move(QString name, QString from, QString to)
{
	if (!QFile::rename(from + "/" + name, to + "/" + name))
	{
		QMessageBox::warning(this, "Error moving file", "Could not move file '" + name + "' from '" +from + "' to '" + to + "'!");
	}
}
