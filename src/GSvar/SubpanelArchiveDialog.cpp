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

	path_subpanel = QFileInfo(NGSD::getTargetFilePath(true)).canonicalFilePath();
	path_archive = QFileInfo(path_subpanel + "/archive/").canonicalFilePath();
	qDebug() << path_subpanel;
	qDebug() << path_archive;
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
	QDesktopServices::openUrl(QUrl(NGSD::getTargetFilePath(true)));
}

void SubpanelArchiveDialog::updateSubpanelLists()
{
	updateSubpanelList(ui->list_subpanel, path_subpanel);
	updateSubpanelList(ui->list_archive, path_archive);
}

void SubpanelArchiveDialog::updateSubpanelList(QListWidget* list, QString path)
{
	list->clear();

	QStringList files = Helper::findFiles(path, "*.bed", false);
	foreach(QString file, files)
	{
		QString name = QFileInfo(file).fileName().replace(".bed", "");
		list->addItem(name);
	}
}

void SubpanelArchiveDialog::archive(QListWidgetItem* item)
{
	if (item==nullptr) return;

	move(item->text() + ".bed", path_subpanel, path_archive);
	move(item->text() + "_genes.txt", path_subpanel, path_archive);

	updateSubpanelLists();
}

void SubpanelArchiveDialog::restore(QListWidgetItem* item)
{
	if (item==nullptr) return;

	move(item->text() + ".bed", path_archive, path_subpanel);
	move(item->text() + "_genes.txt", path_archive, path_subpanel);

	updateSubpanelLists();
}

void SubpanelArchiveDialog::move(QString name, QString from, QString to)
{
	if (!QFile::rename(from + "/" + name, to + "/" + name))
	{
		QMessageBox::warning(this, "Error moving file", "Could not move file '" + name + "' from '" +from + "' to '" + to + "'!");
	}
}
