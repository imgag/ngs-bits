#include "VirusDetectionWidget.h"
#include "GUIHelper.h"
#include "TSVFileStream.h"
#include "GlobalServiceProvider.h"
#include "LoginManager.h"
#include "IgvSessionManager.h"
#include "ClientHelper.h"
#include <QAction>
#include <QMenu>
#include <QMessageBox>

VirusDetectionWidget::VirusDetectionWidget(QString viral_file, QWidget* parent)
	: QTableWidget(parent)
	, viral_file_(viral_file)
	, igv_initialized_(false)
{
	setMinimumSize(700, 500);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setContextMenuPolicy(Qt::CustomContextMenu);

	IgvSessionManager::setIGVInitialized(false, true);
	connect(this, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(callViewInIGV(int, int)));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(callCustomMenu(QPoint)));

	populateTable();
}

void VirusDetectionWidget::populateTable()
{	
	clear();

	setEditTriggers(QAbstractItemView::NoEditTriggers);
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
		setRowCount(row_index + 1);
		if (columnCount() < parts.count()) setColumnCount(parts.count());
		for (int i = 0; i < parts.count(); i++)
		{
			setItem(row_index, i, GUIHelper::createTableItem(parts[i]));
		}
	}

	setHorizontalHeaderLabels(headers_as_strings);

	GUIHelper::resizeTableCells(this, 200);
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
	menu->popup(viewport()->mapToGlobal(pos));
}

void VirusDetectionWidget::callViewInIGV()
{
	if (!selectedItems().isEmpty())
	{
		openInIGV(selectedItems()[0]->row());
	}
}

void VirusDetectionWidget::callCopyToClipboard()
{
	if (!selectedItems().isEmpty())
	{
		GUIHelper::copyToClipboard(this, true);
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
	if (!IgvSessionManager::isIGVInitialized(true))
	{
		foreach (FileLocation file, bam_files)
		{
			commands.append("load \"" + ClientHelper::stripSecureToken(file.filename) + "\"");
		}
	}

	commands.append("goto " + item(row, 0)->text() + ":" + item(row, 1)->text() + "-" + item(row, 2)->text());
	GlobalServiceProvider::executeCommandListInIGV(commands, true, true);
}

void VirusDetectionWidget::keyPressEvent(QKeyEvent* event)
{
	if(event->matches(QKeySequence::Copy))
	{
		GUIHelper::copyToClipboard(this, true);
	}
	else //default key-press event
	{
		QTableWidget::keyPressEvent(event);
	}
}
