#include "GeneOmimInfoWidget.h"
#include "GUIHelper.h"
#include "NGSD.h"
#include <QClipboard>

GeneOmimInfoWidget::GeneOmimInfoWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));
	connect(ui_.genes, SIGNAL(editingFinished()), this, SLOT(updateTable()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
}

void GeneOmimInfoWidget::updateTable()
{
	NGSD db;

	//split genes
	QString genes_str = ui_.genes->text().replace("\t", " ").replace("\n", " ").replace(",", " ").trimmed();
	QStringList genes = genes_str.split(" ");

	//update table
	ui_.table->setRowCount(0);
	foreach(QString gene, genes)
	{
		gene = gene.trimmed();
		if (gene.isEmpty()) continue;

		int row = ui_.table->rowCount();
		ui_.table->setRowCount(ui_.table->rowCount()+1);

		//gene
		ui_.table->setItem(row, 0, GUIHelper::createTableItem(gene));

		//approved symbol
		QString gene_approved = db.geneToApproved(gene.toUtf8(), false);
		ui_.table->setItem(row, 1, GUIHelper::createTableItem(gene_approved));

		//OMIM
		QList<OmimInfo> omim_infos = db.omimInfo(gene.toUtf8());
		for(int i=0; i<omim_infos.count(); ++i)
		{
			if (i>0)
			{
				row = ui_.table->rowCount();
				ui_.table->setRowCount(ui_.table->rowCount()+1);
			}

			const OmimInfo& omim_info = omim_infos[i];
			QLabel* label = GUIHelper::createLinkLabel(link(omim_info.mim, '*')+ " " + omim_info.gene_symbol);
			ui_.table->setCellWidget(row, 2, label);

			QStringList phenos;
            for (const Phenotype& p : omim_info.phenotypes)
			{
				phenos << p.name();
			}

			ui_.table->setItem(row, 3, GUIHelper::createTableItem(phenos.join("\n")));
		}
	}
	ui_.table->resizeRowsToContents();
}

void GeneOmimInfoWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.table);

	//remove HTML tags
	QString text = QApplication::clipboard()->text();
	text.replace("</a>", "");
	text.replace(QRegularExpression("<a .*>"), "");
	QApplication::clipboard()->setText(text);
}

QString GeneOmimInfoWidget::link(QString number, char prefix)
{
	return "<a target=_blank href=\"https://www.omim.org/entry/" + number + "\">" + prefix + number + "</a>";
}
