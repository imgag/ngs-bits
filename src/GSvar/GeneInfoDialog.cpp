#include "GeneInfoDialog.h"
#include "ui_GeneInfoDialog.h"
#include "Helper.h"
#include <QPushButton>

GeneInfoDialog::GeneInfoDialog(QByteArray symbol, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::GeneInfoDialog)
{
	//init dialog
	NGSD db;
	ui->setupUi(this);
	ui->inheritance_->addItems(db.getEnum("geneinfo_germline", "inheritance"));
	ui->notice_->setVisible(false);
	connect(this, SIGNAL(accepted()), this, SLOT(storeGeneInfo()));

	//get gene info
	GeneInfo info = db.geneInfo(symbol);

	//show symbol
	setWindowTitle("Gene information '" + info.symbol + "'");
	ui->gene_->setText(info.symbol);
	ui->name_->setText(info.name);
	ui->inheritance_->setCurrentText(info.inheritance);
	QString tmp = "[" + QDate::currentDate().toString("dd.MM.yyyy") + " " + Helper::userName() + "]\n<font color='red'>Add comment here</font>\n\n";
	tmp.append(info.comments);
	tmp.replace(QRegExp("((?:https?|ftp)://\\S+)"), "<a href=\"\\1\">\\1</a>");
	tmp.replace("\n", "<br>");
	ui->comments_->setHtml(tmp);

	//show gnomAD o/e score
	ui->oe_mis->setText(info.oe_mis);
	ui->oe_syn->setText(info.oe_syn);
	ui->oe_lof->setText(info.oe_lof);

	//show notice if necessary
	if (!info.notice.startsWith("KEPT:"))
	{
		ui->notice_->setText("<font color='red'>" + info.notice + "</font>");
		ui->notice_->setVisible(true);
	}

	//show alias gene symbols from HGNC
	int gene_id = db.geneToApprovedID(symbol);
	ui->previous_->setText(db.previousSymbols(gene_id).join(", "));
	ui->synonymous_->setText(db.synonymousSymbols(gene_id).join(", "));

	//show phenotypes/diseases from HPO
	QByteArrayList pheno_names;
	QList<Phenotype> pheno_list = db.phenotypes(symbol);
	foreach(const Phenotype& pheno, pheno_list)
	{
		pheno_names << pheno.name();
	}
	ui->pheno_->setText(pheno_names.join(", "));

	//disable ok button
	ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
	connect(ui->inheritance_, SIGNAL(currentTextChanged(QString)), this, SLOT(enableOkButton()));
	connect(ui->comments_, SIGNAL(textChanged()), this, SLOT(enableOkButton()));
}

GeneInfoDialog::~GeneInfoDialog()
{
	delete ui;
}

void GeneInfoDialog::enableOkButton()
{
	ui->buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void GeneInfoDialog::storeGeneInfo()
{
	GeneInfo tmp;
	tmp.symbol = ui->gene_->text();
	tmp.inheritance = ui->inheritance_->currentText();
	tmp.oe_syn = ui->oe_syn->text();
	tmp.oe_mis = ui->oe_mis->text();
	tmp.oe_lof = ui->oe_lof->text();
	tmp.comments = ui->comments_->toPlainText();

	NGSD db;
	db.setGeneInfo(tmp);
}
