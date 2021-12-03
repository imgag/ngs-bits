#include "PRSWidget.h"
#include "Helper.h"
#include "GUIHelper.h"
#include <QDesktopServices>
#include <QFile>
#include <QMenu>

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
	QString pgs = ui_.prs->item(row, prs_table_.headers().indexOf("pgs_id"))->text();
	QString pgp = ui_.prs->item(row, prs_table_.headers().indexOf("pgp_id"))->text();


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
		QString citation = ui_.prs->item(row, prs_table_.headers().indexOf("citation"))->text();
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
	const QStringList& headers = prs_table_.headers();
	ui_.prs->setColumnCount(headers.count());
	for (int c=0; c<headers.count(); ++c)
	{
		ui_.prs->setHorizontalHeaderItem(c, GUIHelper::createTableItem(headers[c].trimmed()));
	}

	//add contents
	ui_.prs->setRowCount(prs_table_.rowCount());
	for (int r=0; r<prs_table_.rowCount(); ++r)
	{
		for (int c=0; c<headers.count(); ++c)
		{
			ui_.prs->setItem(r , c, GUIHelper::createTableItem(prs_table_.row(r)[c].trimmed()));
		}
	}

	GUIHelper::resizeTableCells(ui_.prs);
}
