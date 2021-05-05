#include "GeneWidget.h"
#include "Helper.h"
#include "SmallVariantSearchWidget.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include "GeneInfoDBs.h"
#include "GSvarHelper.h"
#include <QPushButton>
#include <QInputDialog>
#include <QMenu>
#include <QDesktopServices>

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
    connect(ui_.pseudogenes, SIGNAL(linkActivated(QString)), this, SLOT(parseLink(QString)));
	connect(ui_.type, SIGNAL(linkActivated(QString)), this, SIGNAL(openGeneTab(QString)));

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
	ui_.type->setText(info.locus_group);
	ui_.inheritance->setText(info.inheritance);
    QString html = info.comments;
    html.replace(QRegExp("((?:https?|ftp)://\\S+)"), "<a href=\"\\1\">\\1</a>");
    html.replace("\n", "<br>");
	ui_.comments->setText(html);

    //add pseudogenes/parent genes
	int gene_id = db.geneToApprovedID(symbol_);
	QStringList pseudogene_link_list;
	QStringList parent_gene_link_list;
	QStringList pseudogene_ids = db.getValues("SELECT pseudogene_gene_id FROM gene_pseudogene_relation WHERE parent_gene_id=" + QString::number(gene_id) + " AND pseudogene_gene_id IS NOT NULL");
	foreach (const QString& pseudogene_id, pseudogene_ids)
	{
		QString pseudogene_name = db.geneSymbol(pseudogene_id.toInt());
		pseudogene_link_list.append("<a href=\"" + pseudogene_name + "\">" + pseudogene_name + "</a>");
	}
	QStringList pseudogenes = db.getValues("SELECT gene_name FROM gene_pseudogene_relation WHERE parent_gene_id=" + QString::number(gene_id) + " AND gene_name IS NOT NULL");
	foreach (const QString& pseudogene, pseudogenes)
	{
		QString ensembl_id = pseudogene.split(';').at(0);
		QString pseudogene_name = pseudogene.split(';').at(1);
		pseudogene_link_list.append(pseudogene_name + " (<a href=\"ensembl:" + ensembl_id + "\">"+ ensembl_id + ")</a>");
	}
	if (pseudogene_link_list.size() > 0)
	{
		ui_.pseudogenes->setText(pseudogene_link_list.join(", "));
	}
	else if (ui_.pseudogenes!=nullptr)
	{
		ui_.pseudogenes->deleteLater();
		ui_.pseudogenes = nullptr;
		ui_.pseudogene_label->deleteLater();
		ui_.pseudogene_label = nullptr;
	}

	// add parent gene (for pseudogenes)
    QStringList parent_gene_ids = db.getValues("SELECT parent_gene_id FROM gene_pseudogene_relation WHERE pseudogene_gene_id=" + QString::number(gene_id) + " AND parent_gene_id IS NOT NULL");
    foreach (const QString& parent_gene_id, parent_gene_ids)
    {
        QString parent_gene_name = db.geneSymbol(parent_gene_id.toInt());
        parent_gene_link_list.append("<a href=\"" + parent_gene_name + "\">" + parent_gene_name + "</a>");
    }
    if (parent_gene_link_list.size() > 0)
    {
		ui_.type->setText(ui_.type->text() + " (Parent gene: " + parent_gene_link_list.join(", ") + ")");
    }

	//add imprinting infos
	if (!info.imprinting_status.isEmpty() || !info.imprinting_source_allele.isEmpty())
	{
		ui_.imprinting->setText(info.imprinting_source_allele + " (" + info.imprinting_status + ")");
	}
	else
	{
		ui_.imprinting->deleteLater();
		ui_.imprinting = nullptr;
		ui_.imprinting_label->deleteLater();
		ui_.imprinting_label = nullptr;
	}

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
	ui_.hgnc_id->setText("<a href='https://www.genenames.org/data/gene-symbol-report/#!/hgnc_id/" + info.hgnc_id + "'>" + info.hgnc_id + "</a>");
	ui_.hgnc_previous->setText(db.previousSymbols(gene_id).join(", "));
	ui_.hgnc_synonymous->setText(db.synonymousSymbols(gene_id).join(", "));

	//show phenotypes/diseases from HPO
    QByteArrayList hpo_links;
	PhenotypeList pheno_list = db.phenotypes(symbol_);
	foreach(const Phenotype& pheno, pheno_list)
	{
        hpo_links << "<a href=\"https://hpo.jax.org/app/browse/term/" + pheno.accession()+ "\">" + pheno.accession() + "</a> " + pheno.name();
	}
	ui_.hpo->setText(hpo_links.join(hpo_links.count()>20 ? " "  : "<br>"));

    //show OMIM info
	QStringList omim_lines;
	QList<OmimInfo> omim_infos = db.omimInfo(symbol_);
	foreach(const OmimInfo& omim, omim_infos)
	{
		QStringList omim_phenos;
		foreach(const Phenotype& p, omim.phenotypes)
		{
			omim_phenos << p.name();
		}
		omim_lines << ("<a href=\"http://omim.org/entry/" + omim.mim + "\">MIM *" + omim.mim + "</a>:<br>" + omim_phenos.join(omim_phenos.count()>20 ? " "  : "<br>"));
    }
	ui_.omim->setText(omim_lines.join("<br>"));

	//show OrphaNet info
	QByteArrayList orpha_links;
	SqlQuery query = db.getQuery();
	query.exec("SELECT dt.* FROM disease_term dt, disease_gene dg WHERE dg.disease_term_id=dt.id AND dt.source='OrphaNet' AND dg.gene='" + symbol_ + "'");
	while (query.next())
	{
		QByteArray identifier = query.value("identifier").toByteArray();
		QByteArray number = identifier.mid(6);
		QByteArray name = query.value("name").toByteArray();
		orpha_links << ("<a href=\"https://www.orpha.net/consor/cgi-bin/OC_Exp.php?Expert=" + number + "\">" + identifier + "</a><br>" + name);
	}
	ui_.diseases->setText(orpha_links.join(orpha_links.count()>20 ? " "  : "<br>"));

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
	SmallVariantSearchWidget* widget = new SmallVariantSearchWidget();
	widget->setGene(symbol_);
	QSharedPointer<QDialog> dlg = GUIHelper::createDialog(widget, "Small variants for " + symbol_);
	dlg->exec();
}

void GeneWidget::openGeneDatabase()
{
	QToolButton* btn = qobject_cast<QToolButton*>(sender());
    GeneInfoDBs::openUrl(btn->toolTip(), symbol_);
}

void GeneWidget::parseLink(QString link)
{
    if (link.startsWith("ensembl:"))
    {
        QString ensembl_id = link.split(':').at(1);
		QString url = "http://grch37.ensembl.org/Homo_sapiens/Transcript/Summary?g=" + ensembl_id;
        QDesktopServices::openUrl(QUrl(url));
    }
    else
    {
        emit openGeneTab(link);
    }
}

void GeneWidget::updateTranscriptsTable(NGSD& db)
{
	//clear
	ui_.transcripts->setRowCount(0);
	const QMap<QByteArray, QByteArrayList>&  matches = GSvarHelper::transcriptMatches();

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

		QString pt = "";
		if (GSvarHelper::preferredTranscripts().value(symbol_).contains(transcript.name())) pt = "yes";
		ui_.transcripts->setItem(row, 5, GUIHelper::createTableItem(pt));

		QStringList ccds;
		QStringList refseq;
		QByteArrayList tmp = matches.value(transcript.name());
		foreach(QByteArray match, tmp)
		{
			match = match.trimmed();
			if (match.startsWith("CCDS"))
			{
				ccds << "<a href='https://www.ncbi.nlm.nih.gov/CCDS/CcdsBrowse.cgi?REQUEST=CCDS&DATA=" + match + "'>" + match + "</a>";
			}
			else if (match.startsWith("NM_"))
			{
				refseq << "<a href='https://www.ncbi.nlm.nih.gov/nuccore/" + match + "'>" + match + "</a>";
			}
		}

		ui_.transcripts->setCellWidget(row, 6, GUIHelper::createLinkLabel(ccds.join(", ")));
		ui_.transcripts->setCellWidget(row, 7, GUIHelper::createLinkLabel(refseq.join(", ")));
	}

	GUIHelper::resizeTableCells(ui_.transcripts);
}
