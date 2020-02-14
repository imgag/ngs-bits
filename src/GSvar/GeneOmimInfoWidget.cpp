#include "GeneOmimInfoWidget.h"
#include "GUIHelper.h"
#include "NGSD.h"
#include <QDesktopServices>
#include <QTextDocument>
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
		QString gene_approved = db.geneToApproved(gene.toLatin1(), false);
		ui_.table->setItem(row, 1, GUIHelper::createTableItem(gene_approved));

		//OMIM gene
		QString omim_gene_id = db.getValue("SELECT id FROM omim_gene WHERE gene=:0", true, gene_approved).toString();
		if (omim_gene_id=="")
		{
			omim_gene_id = db.getValue("SELECT id FROM omim_gene WHERE gene=:0", true, gene).toString();
		}
		if (omim_gene_id!="")
		{
			QString mim = db.getValue("SELECT mim FROM omim_gene WHERE id=" + omim_gene_id).toString();
			QString gene_omim = db.getValue("SELECT gene FROM omim_gene WHERE id=" + omim_gene_id).toString();

			QLabel* label = new QLabel(link(mim, '*')+ " " + gene_omim);
			label->setAlignment(Qt::AlignLeft|Qt::AlignTop);
			label->setOpenExternalLinks(true);
			connect(label, SIGNAL(linkActivated(QString)), this, SLOT(openLink(QString)));

			ui_.table->setCellWidget(row, 2, label);
		}

		//OMIM phenotypes
		if (omim_gene_id!="")
		{
			QStringList phenos = db.getValues("SELECT phenotype FROM omim_phenotype WHERE omim_gene_id=" + omim_gene_id);

			ui_.table->setItem(row, 3, GUIHelper::createTableItem(phenos.join("\n")));
		}
	}
	ui_.table->resizeRowsToContents();
}

void GeneOmimInfoWidget::openLink(QString url)
{
	QDesktopServices::openUrl(QUrl(url));
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
