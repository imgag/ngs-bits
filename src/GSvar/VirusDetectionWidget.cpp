#include "VirusDetectionWidget.h"
#include "GUIHelper.h"
#include "TSVFileStream.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include <QAction>
#include <QMenu>
#include <QMessageBox>

VirusDetectionWidget::VirusDetectionWidget(QString viral_file, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, viral_file_(viral_file)
	, igv_initialized_(false)
{
	ui_.setupUi(this);
	populateTable();
	connect(ui_.virus_table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(callViewInIGV(int, int)));
	connect(ui_.virus_table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(callCustomMenu(QPoint)));
}

void VirusDetectionWidget::populateTable()
{	
	ui_.virus_table->clear();
	ui_.virus_table->setContextMenuPolicy(Qt::CustomContextMenu);

	ui_.virus_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	TSVFileStream stream(viral_file_);
	QByteArrayList headers = stream.header();
	QStringList headers_as_strings;
	foreach (QByteArray item, headers)
	{
		headers_as_strings.append(QString::fromLocal8Bit(item));
	}

	int row_index = -1;
	while(!stream.atEnd())
	{
		QByteArrayList parts = stream.readLine();
		if (parts.isEmpty()) continue;
		row_index++;
		ui_.virus_table->setRowCount(row_index + 1);
		if (ui_.virus_table->columnCount() < parts.count()) ui_.virus_table->setColumnCount(parts.count());
		for (int i = 0; i < parts.count(); i++)
		{
			ui_.virus_table->setItem(row_index, i, GUIHelper::createTableItem(parts[i]));
		}
	}

	ui_.virus_table->setHorizontalHeaderLabels(headers_as_strings);
}

void VirusDetectionWidget::callViewInIGV(int row, int /*col*/)
{
	openInIGV(row);
}

void VirusDetectionWidget::callCustomMenu(QPoint pos)
{
	QMenu * menu = new QMenu(this);
	QAction* open_in_igv =  new QAction(QIcon(":/Icons/IGV.png"), "Open in IGV");
	QAction* copy_to_clipboard = new QAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selected rows to clipboard");

	connect(open_in_igv, SIGNAL(triggered()), this, SLOT(callViewInIGV()));
	connect(copy_to_clipboard, SIGNAL(triggered()), this, SLOT(callCopyToClipboard()));

	menu->addAction(open_in_igv);
	menu->addAction(copy_to_clipboard);
	menu->popup(ui_.virus_table->viewport()->mapToGlobal(pos));
}

void VirusDetectionWidget::callViewInIGV()
{
	if (!ui_.virus_table->selectedItems().isEmpty())
	{
		openInIGV(ui_.virus_table->selectedItems()[0]->row());
	}
}

void VirusDetectionWidget::callCopyToClipboard()
{
	if (!ui_.virus_table->selectedItems().isEmpty())
	{
		GUIHelper::copyToClipboard(ui_.virus_table, true);
	}
}

void VirusDetectionWidget::openInIGV(int row)
{
	FileLocationList bam_files = GlobalServiceProvider::fileLocationProvider().getViralBamFiles(false);
	if (bam_files.isEmpty())
	{
		QMessageBox::information(this, "BAM files not found", "There are no BAM files associated with the virus!");
		return;
	}

	QStringList commands;
	if (!igv_initialized_)
	{
		commands.append("genome " + NGSHelper::serverApiUrl() + "genome/somatic_viral.fa");
		commands.append("new");
		if (NGSHelper::isClientServerMode()) commands.append("SetAccessToken " + LoginManager::userToken() + " *" + Settings::string("server_host") + "*");
		commands.append("collapse");
		foreach (FileLocation file, bam_files)
		{
			commands.append("load \"" + NGSHelper::stripSecureToken(file.filename) + "\"");
		}
		commands.append("viewaspairs");
		commands.append("colorBy UNEXPECTED_PAIR");
		igv_initialized_ = true;
	}

	commands.append("goto " + ui_.virus_table->item(row, 0)->text() + ":" + ui_.virus_table->item(row, 1)->text() + "-" + ui_.virus_table->item(row, 2)->text());
	GlobalServiceProvider::executeCommandListInIGV(commands, false);
}
