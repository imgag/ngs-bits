#include "GeneInfoDialog.h"
#include "ui_GeneInfoDialog.h"
#include "Helper.h"
#include <QPushButton>

GeneInfoDialog::GeneInfoDialog(QString symbol, QWidget *parent)
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
	ui->inheritance_->setCurrentText(info.inheritance);
	QString tmp = "[" + QDate::currentDate().toString("dd.MM.yyyy") + " " + Helper::userName() + "]\n<font color='red'>Add comment here</font>\n\n";
	tmp.append(info.comments);
	tmp.replace(QRegExp("((?:https?|ftp)://\\S+)"), "<a href=\"\\1\">\\1</a>");
	tmp.replace("\n", "<br>");
	ui->comments_->setHtml(tmp);

	//show ExAC pLI score
	ui->exac_pli->setText(info.exac_pli);

	//show notice if necessary
	if (!info.notice.startsWith("KEPT:"))
	{
		ui->notice_->setText("<font color='red'>" + info.notice + "</font>");
		ui->notice_->setVisible(true);
	}

	//show alias gene symbols from HGNC
	ui->previous_->setText(db.previousSymbols(symbol).join(", "));
	ui->synonymous_->setText(db.synonymousSymbols(symbol).join(", "));

	//show phenotypes/diseases from HPO
	ui->pheno_->setText(db.phenotypes(symbol).join(", "));

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
	tmp.exac_pli = ui->exac_pli->text();
	tmp.comments = ui->comments_->toPlainText();

	NGSD db;
	db.setGeneInfo(tmp);
}
