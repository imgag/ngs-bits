#include "GeneWidget.h"
#include "Helper.h"
#include "CandidateGeneDialog.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include "GeneInfoDBs.h"
#include <QPushButton>
#include <QInputDialog>
#include <QMenu>

GeneWidget::GeneWidget(QWidget* parent, QByteArray symbol)
    : QWidget(parent)
    , ui_()
    , symbol_(symbol)
{
	//init dialog
    ui_.setupUi(this);
	ui_.notice->setVisible(false);
    connect(ui_.refesh_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_.variation_btn, SIGNAL(clicked(bool)), this, SLOT(showGeneVariationDialog()));

    //edit button
    QMenu* menu = new QMenu();
    menu->addAction("Edit inheritance", this, SLOT(editInheritance()));
    menu->addAction("Edit comment", this, SLOT(editComment()));
    ui_.edit_btn->setMenu(menu);

	//gene database buttons
	QHBoxLayout* layout = ui_.gene_button_layout;
	foreach(const GeneDB& db, GeneInfoDBs::all())
	{
		QToolButton* btn = new QToolButton();
		btn->setToolTip(db.name);
		btn->setIcon(db.icon);
		layout->addWidget(btn);
		connect(btn, SIGNAL(clicked(bool)), this, SLOT(openGeneDatabase()));
	}

    updateGUI();
}

void GeneWidget::updateGUI()
{
    //get gene info
	NGSD db;
    GeneInfo info = db.geneInfo(symbol_);

	//show symbol
	setWindowTitle("Gene information '" + info.symbol + "'");
	ui_.gene->setText(info.symbol);
	ui_.name->setText(info.name);
	ui_.inheritance->setText(info.inheritance);
    QString html = info.comments;
    html.replace(QRegExp("((?:https?|ftp)://\\S+)"), "<a href=\"\\1\">\\1</a>");
    html.replace("\n", "<br>");
	ui_.comments->setText(html);

	//show gnomAD o/e score
    ui_.oe_mis->setText(info.oe_mis);
    ui_.oe_syn->setText(info.oe_syn);
    ui_.oe_lof->setText(info.oe_lof);

	//show notice if necessary
    if (!info.symbol_notice.startsWith("KEPT:"))
	{
		ui_.notice->setText("<font color='red'>" + info.symbol_notice + "</font>");
		ui_.notice->setVisible(true);
	}

	//HGNC info
    int gene_id = db.geneToApprovedID(symbol_);
	QString hgnc_id = "HGNC:" + db.getValue("SELECT hgnc_id FROM gene WHERE id=" + QByteArray::number(gene_id)).toString();
	ui_.hgnc_id->setText("<a href='https://www.genenames.org/data/gene-symbol-report/#!/hgnc_id/" + hgnc_id + "'>" + hgnc_id + "</a>");
	ui_.hgnc_previous->setText(db.previousSymbols(gene_id).join(", "));
	ui_.hgnc_synonymous->setText(db.synonymousSymbols(gene_id).join(", "));

	//show phenotypes/diseases from HPO
    QByteArrayList hpo_links;
    QList<Phenotype> pheno_list = db.phenotypes(symbol_);
	foreach(const Phenotype& pheno, pheno_list)
	{
        hpo_links << "<a href=\"https://hpo.jax.org/app/browse/term/" + pheno.accession()+ "\">" + pheno.accession() + "</a> " + pheno.name();
	}
	ui_.hpo->setText(hpo_links.join(hpo_links.count()>20 ? " "  : "\n"));

    //show OMIM info
	OmimInfo omim = db.omimInfo(symbol_);
	if (!omim.gene_symbol.isEmpty())
	{
		QStringList omim_phenos;
		foreach(const Phenotype& p, omim.phenotypes)
		{
			omim_phenos << p.name();
		}
		ui_.omim->setText("MIM: <a href=\"http://omim.org/entry/" + omim.mim + "\">" + omim.mim + "</a>\n" + omim_phenos.join(omim_phenos.count()>20 ? " "  : "\n"));
    }

	//show OrphaNet info
	QByteArrayList orpha_links;
	SqlQuery query = db.getQuery();
	query.exec("SELECT dt.* FROM disease_term dt, disease_gene dg WHERE dg.disease_term_id=dt.id AND dg.gene='" + symbol_ + "'");
	if (query.next())
	{
		QByteArray identifier = query.value("identifier").toByteArray();
		QByteArray number = identifier.mid(6);
		QByteArray name = query.value("name").toByteArray();
		orpha_links << ("<a href=\"https://www.orpha.net/consor/cgi-bin/OC_Exp.php?Expert=" + number + "\">" + identifier + "</a>\n" + name);
	}
	ui_.diseases->setText(orpha_links.join(orpha_links.count()>20 ? " "  : "\n"));

	updateTranscriptsTable(db);
}

void GeneWidget::editInheritance()
{
    NGSD db;
    GeneInfo info = db.geneInfo(symbol_);
    QStringList modes = db.getEnum("geneinfo_germline", "inheritance");

    bool ok;
    QString mode_new = QInputDialog::getItem(this, "Select gene inheritance mode", "inheritance:", modes, modes.indexOf(info.inheritance), false, &ok);
    if (!ok) return;

    info.inheritance = mode_new;
    db.setGeneInfo(info);

    updateGUI();
}

void GeneWidget::editComment()
{
    NGSD db;
    GeneInfo info = db.geneInfo(symbol_);
	QString text = "[" + QDate::currentDate().toString("dd.MM.yyyy") + " " + LoginManager::user() + "]\nAdd comment here\n\n" + info.comments;

    bool ok;
    QString text_new = QInputDialog::getMultiLineText(this, "Edit gene comment", "comment:", text, &ok);
    if (!ok) return;

    info.comments = text_new;
    db.setGeneInfo(info);

    updateGUI();
}

void GeneWidget::showGeneVariationDialog()
{
	CandidateGeneDialog dlg(this);
	dlg.setGene(symbol_);
	dlg.exec();
}

void GeneWidget::openGeneDatabase()
{
	QToolButton* btn = qobject_cast<QToolButton*>(sender());
	GeneInfoDBs::openUrl(btn->toolTip(), symbol_);
}

void GeneWidget::updateTranscriptsTable(NGSD& db)
{
	//clear
	ui_.transcripts->setRowCount(0);

	//get transcripts
	int gene_id = db.geneToApprovedID(symbol_);
	QList<Transcript> transcripts = db.transcripts(gene_id, Transcript::ENSEMBL, false);

	//sort transcripts
	std::stable_sort(transcripts.begin(), transcripts.end(), [](const Transcript& a, const Transcript& b){ return a.regions().baseCount() > b.regions().baseCount(); });
	std::stable_sort(transcripts.begin(), transcripts.end(), [](const Transcript& a, const Transcript& b){ return a.codingRegions().baseCount() > b.codingRegions().baseCount(); });

	//display
	foreach(const Transcript& transcript, transcripts)
	{
		int row = ui_.transcripts->rowCount();
		ui_.transcripts->setRowCount(row+1);

		QLabel* label = GUIHelper::createLinkLabel("<a href='http://grch37.ensembl.org/Homo_sapiens/Transcript/Summary?t=" + transcript.name() + "'>" + transcript.name() + "</a>");
		ui_.transcripts->setCellWidget(row, 0, label);

		QString coords = "";
		int reg_count = transcript.regions().count();
		if (reg_count>0)
		{
			coords = transcript.regions()[0].chr().strNormalized(true) + ":" + QString::number(transcript.regions()[0].start()) + "-" + QString::number(transcript.regions()[reg_count-1].end());
		}
		ui_.transcripts->setItem(row, 1, GUIHelper::createTableItem(coords));

		QString bases = QString::number(transcript.regions().baseCount());
		ui_.transcripts->setItem(row, 2, GUIHelper::createTableItem(bases));

		QString coding_bases_exons = "";
		if (transcript.isCoding()) coding_bases_exons = QString::number(transcript.codingRegions().baseCount()-3) + " / " + QString::number(transcript.codingRegions().count());
		ui_.transcripts->setItem(row, 3, GUIHelper::createTableItem(coding_bases_exons));

		QString aas = "no protein";
		if (transcript.isCoding()) aas = QString::number(transcript.codingRegions().baseCount()/3-1);
		ui_.transcripts->setItem(row, 4, GUIHelper::createTableItem(aas));
	}

	GUIHelper::resizeTableCells(ui_.transcripts);
}
