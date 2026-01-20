#include "PRSWidget.h"
#include "Helper.h"
#include "GUIHelper.h"
#include <QDesktopServices>
#include <QMenu>
#include <QUrl>

PRSWidget::PRSWidget(QString filename, QWidget *parent)
	: QWidget(parent)
	, ui_()
	, prs_table_()
{
	ui_.setupUi(this);

	// connect signals and slots
	connect(ui_.prs,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

	prs_table_.load(filename);
	initGui();
}

void PRSWidget::showContextMenu(QPoint pos)
{
	int row = ui_.prs->itemAt(pos)->row();
	QString pgs = ui_.prs->item(row, prs_table_.columnIndex("pgs_id"))->text();
	QString pgp = ui_.prs->item(row, prs_table_.columnIndex("pgp_id"))->text();


	//create context menu based on PRS entry
	QMenu menu(ui_.prs);

	QAction* a_pgs = nullptr;
	QAction* a_pgp = nullptr;
	if (pgs.startsWith("PGS")) a_pgs = menu.addAction(QIcon(":/Icons/PGSCatalog.png"), "Show score on PGS Catalog");
	if (pgp.startsWith("PGP")) a_pgp = menu.addAction(QIcon(":/Icons/PGSCatalog.png"), "Show publication on PGS Catalog");

	QAction* a_citation = menu.addAction("Show publication");

	//execute menu
	QAction* action = menu.exec(ui_.prs->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;

	if (action == a_pgs)
	{
		QDesktopServices::openUrl(QUrl("https://www.pgscatalog.org/score/" + pgs));
	}
	else if (action == a_pgp)
	{
		QDesktopServices::openUrl(QUrl("https://www.pgscatalog.org/publication/" + pgp));
	}
	else if (action == a_citation)
	{
		QString citation = ui_.prs->item(row, prs_table_.columnIndex("citation"))->text();
		if (citation.startsWith("CanRisk ("))
		{
			//special handling of CanRisk PRS scores
			QString url = citation.split('(').at(1).split(')').at(0);
			QDesktopServices::openUrl(QUrl(url));
		}
		else
		{
			QString doi = citation.split("doi:").at(1).trimmed();
			QDesktopServices::openUrl(QUrl("https://doi.org/" + doi));
		}

	}
}

void PRSWidget::initGui()
{
	//set header
	ui_.prs->setColumnCount(prs_table_.columnCount());
	for (int c=0; c<prs_table_.columnCount(); ++c)
	{
		ui_.prs->setHorizontalHeaderItem(c, GUIHelper::createTableItem(prs_table_.headers()[c].trimmed()));
	}

	//add contents
	ui_.prs->setRowCount(prs_table_.count());
	for (int r=0; r<prs_table_.count(); ++r)
	{
		for (int c=0; c<prs_table_.columnCount(); ++c)
		{
			ui_.prs->setItem(r , c, GUIHelper::createTableItem(prs_table_[r][c].trimmed()));
		}
	}

	//color low depth column if too many variants are low depth
	int c_all = GUIHelper::columnIndex(ui_.prs, "variants_in_prs");
	int c_low_depth = GUIHelper::columnIndex(ui_.prs, "variants_low_depth");
	for (int r=0; r<prs_table_.count(); ++r)
	{
		int all = Helper::toInt(ui_.prs->item(r, c_all)->text(), "variants_in_prs");
		int low_depth = Helper::toInt(ui_.prs->item(r, c_low_depth)->text(), "variants_low_depth");
		if (low_depth>=0.1*all) ui_.prs->item(r, c_low_depth)->setBackground(Qt::yellow);
	}

	GUIHelper::resizeTableCellWidths(ui_.prs);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.prs);
}
