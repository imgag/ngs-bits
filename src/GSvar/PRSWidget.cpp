#include "PRSWidget.h"
#include "ui_PRSWidget.h"
#include "Helper.h"
#include "GUIHelper.h"
#include <QDesktopServices>
#include <QFile>
#include <QMenu>

PRSWidget::PRSWidget(QString filename, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::PRSView),
	filename_(filename)
{
	ui_->setupUi(this);
	ui_->prs->setContextMenuPolicy(Qt::CustomContextMenu);

	// connect signals and slots
	connect(ui_->prs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

	loadPrsData();
}

PRSWidget::~PRSWidget()
{
	delete ui_;
}

void PRSWidget::showContextMenu(QPoint pos)
{
	qDebug() << "create ContextMenu";
	int row = ui_->prs->itemAt(pos)->row();

	//create context menu
	QMenu menu(ui_->prs);

	QAction* a_pgs = menu.addAction(QIcon(":/Icons/PGSCatalog.png"), "Show score on PGS Catalog");
	QAction* a_pgp = menu.addAction(QIcon(":/Icons/PGSCatalog.png"), "Show publication on PGS Catalog");
	QAction* a_citation = menu.addAction("Show publication");

	//execute menu
	QAction* action = menu.exec(ui_->prs->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;

	if (action == a_pgs)
	{
		QString pgs = ui_->prs->item(row, column_header_.indexOf("pgs_id"))->text();
		QDesktopServices::openUrl(QUrl("https://www.pgscatalog.org/score/" + pgs));
	}
	else if (action == a_pgp)
	{
		QString pgp = ui_->prs->item(row, column_header_.indexOf("pgp_id"))->text();
		QDesktopServices::openUrl(QUrl("https://www.pgscatalog.org/publication/" + pgp));
	}
	else if (action == a_citation)
	{
		QString doi = ui_->prs->item(row, column_header_.indexOf("citation"))->text().split("doi:").at(1).trimmed();
		QDesktopServices::openUrl(QUrl("https://doi.org/" + doi));
	}
}

void PRSWidget::loadPrsData()
{
	// load tsv file
	QSharedPointer<QFile> input_file = Helper::openFileForReading(filename_, false);

	int row_idx = 0;
	int column_count = 0;
	while(!input_file->atEnd())
	{
		QByteArray line = input_file -> readLine();

		// skip empty lines and comments
		if (line.startsWith("##") || line.trimmed() == "") continue;

		// parse header
		if (line.startsWith('#'))
		{
			column_header_ = line.mid(1).trimmed().split('\t');
			column_count = column_header_.size();

			//create header items
			ui_->prs->setColumnCount(column_count);
			for (int col_idx = 0; col_idx < column_count; ++col_idx)
			{
				ui_->prs->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem(QString(column_header_.at(col_idx).trimmed())));
			}
			continue;
		}

		// parse data line
		QByteArrayList split_line = line.split('\t');

		// add table row
		ui_->prs->setRowCount(row_idx + 1);

		// fill row
		for (int col_idx = 0; col_idx < column_count; ++col_idx)
		{
			QString text = split_line.at(col_idx).trimmed();
			ui_->prs->setItem(row_idx , col_idx, GUIHelper::createTableItem(text));
		}
		row_idx++;
	}

	//check if all required columns are available
	QByteArrayList required_columns;
	required_columns << "pgs_id" << "trait" << "score" << "percentile" << "build" << "pgp_id" << "citation";
	foreach (const QByteArray& required_column, required_columns)
	{
		if (!column_header_.contains(required_column))
		{
			THROW(FileParseException, "Required column '" + required_column + "' not found in PRS file!");
		}
	}

	GUIHelper::resizeTableCells(ui_->prs);

}
