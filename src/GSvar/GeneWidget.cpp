#include "GeneWidget.h"
#include "Helper.h"
#include "SmallVariantSearchWidget.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include "GeneInfoDBs.h"
#include "GSvarHelper.h"
#include "GlobalServiceProvider.h"
#include <QInputDialog>
#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>

GeneWidget::GeneWidget(QWidget* parent, QByteArray symbol)
	: TabBaseClass(parent)
    , ui_()
    , init_timer_(this, true)
    , symbol_(symbol)
{
    //init dialog
    ui_.setupUi(this);
	ui_.notice->setVisible(false);
    connect(ui_.refesh_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_.variation_btn, SIGNAL(clicked(bool)), this, SLOT(showGeneVariationDialog()));
    connect(ui_.pseudogenes, SIGNAL(linkActivated(QString)), this, SLOT(parseLink(QString)));
	connect(ui_.type, SIGNAL(linkActivated(QString)), this, SLOT(openGeneTab(QString)));
	connect(ui_.pheno_search, SIGNAL(editingFinished()), this, SLOT(updatePhenotypeSearch()));

    //edit button
    QMenu* menu = new QMenu();
    menu->addAction("Edit inheritance", this, SLOT(editInheritance()));
    menu->addAction("Edit comment", this, SLOT(editComment()));
    ui_.edit_btn->setMenu(menu);
}

void GeneWidget::delayedInitialization()
{
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
	is_busy_ = true;

	try
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
		html.replace(QRegularExpression("((?:https?|ftp)://\\S+)"), "<a href=\"\\1\">\\1</a>");
		GSvarHelper::limitLines(ui_.comments, html);

		//ids
		int gene_id = db.geneId(symbol_);
		QStringList ids;
		QString hgnc_id = info.hgnc_id.replace("HGNC:", "");
		ids << ("HGNC:<a href='https://www.genenames.org/data/gene-symbol-report/#!/hgnc_id/" + hgnc_id + "'>" + hgnc_id + "</a>");
		QString ensembl_id = db.getValue("SELECT ensembl_id FROM gene WHERE id="+QString::number(gene_id)).toString().trimmed();
		if (!ensembl_id.isEmpty())
		{
			ids << ("Ensembl:<a href='http://www.ensembl.org/Homo_sapiens/Gene/Summary?g=" + ensembl_id + "'>" + ensembl_id + "</a>");
		}
		QString ncbi_id = db.getValue("SELECT ncbi_id FROM gene WHERE id="+QString::number(gene_id)).toString().trimmed();
		if (!ncbi_id.isEmpty())
		{
			ids << ("NCBI:<a href='https://www.ncbi.nlm.nih.gov/gene/" + ncbi_id + "'>" + ncbi_id + "</a>");
		}
		ui_.ids->setText(ids.join(" "));

		//add pseudogenes/parent genes
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
		if (!info.imprinting_confidence.isEmpty() || !info.imprinting_expressed_allele.isEmpty())
		{
			ui_.imprinting->setText("expressed allele: " + info.imprinting_expressed_allele + " (confidence: " + info.imprinting_confidence + ")");
		}
		else
		{
			if (ui_.imprinting!=nullptr)
			{
				ui_.imprinting->deleteLater();
				ui_.imprinting = nullptr;
			}
			if (ui_.imprinting_label!=nullptr)
			{
				ui_.imprinting_label->deleteLater();
				ui_.imprinting_label = nullptr;
			}
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

		//HGNC previous/synonymous symbols
		ui_.hgnc_previous->setText(db.previousSymbols(gene_id).join(", "));
		ui_.hgnc_synonymous->setText(db.synonymousSymbols(gene_id).join(", "));

		//check if there is CSpec data available
		QList<int> cspec_ids = db.getValuesInt("SELECT id FROM cspec_data WHERE gene='"+symbol_+"'");
		ui_.cspec->setText(cspec_ids.isEmpty() ? "No criteria available" : "<b>Criteria available, see</b> <a href='https://cspec.genome.network/cspec/ui/svi/'>Criteria Specification Registry</a>");

		//show phenotypes/diseases from HPO
		hpo_lines.clear();
		PhenotypeList pheno_list = db.phenotypes(symbol_);
		for (const Phenotype& pheno : std::as_const(pheno_list))
		{
			int pheno_id = db.phenotypeIdByAccession(pheno.accession());
			QSet<QString> sources;
			foreach(QString details, db.getValues("SELECT details FROM hpo_genes WHERE gene='" + symbol_ +"' AND hpo_term_id=" + QString::number(pheno_id)))
			{
				QStringList source_parts = details.split(';');
				foreach(QString source_part, source_parts)
				{
					source_part = source_part.trimmed();
					source_part = source_part.mid(1, source_part.length()-2);
					sources << source_part.split(',').at(0);
				}

			}
			hpo_lines << "<a href=\"https://hpo.jax.org/app/browse/term/" + pheno.accession()+ "\">" + pheno.accession() + "</a> " + pheno.name() + " (sources: " + sources.values().join(", ") + ")";
		}
		ui_.hpo->setText(hpo_lines.join("<br>"));

		//show OMIM info
		omim_lines.clear();
		QList<OmimInfo> omim_infos = db.omimInfo(symbol_);
		for (const OmimInfo& omim : std::as_const(omim_infos))
		{
			QStringList omim_phenos;
			for (const Phenotype& p : omim.phenotypes)
			{
				omim_phenos << p.name();
			}
			omim_lines << ("<a href=\"https://omim.org/entry/" + omim.mim + "\">MIM *" + omim.mim + "</a>:<br>" + omim_phenos.join(omim_phenos.count()>20 ? " "  : "<br>"));
		}
		ui_.omim->setText(omim_lines.join("<br>"));

		//show OrphaNet info
		orpha_lines.clear();
		SqlQuery query = db.getQuery();
		query.exec("SELECT dt.* FROM disease_term dt, disease_gene dg WHERE dg.disease_term_id=dt.id AND dt.source='OrphaNet' AND dg.gene='" + symbol_ + "'");
		while (query.next())
		{
			QByteArray identifier = query.value("identifier").toByteArray();
			QByteArray number = identifier.mid(6);
			QByteArray name = query.value("name").toByteArray();
			orpha_lines << ("<a href=\"https://www.orpha.net/consor/cgi-bin/OC_Exp.php?Expert=" + number + "\">" + identifier + "</a> " + name);
		}
		ui_.diseases->setText(orpha_lines.join("<br>"));

		updateTranscriptsTable(db);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Error", "Could not update data:\n" + e.message());
	}

	is_busy_ = false;
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
	QString text = "[" + QDate::currentDate().toString("dd.MM.yyyy") + " " + LoginManager::userLogin() + "]\nAdd comment here\n\n" + info.comments;

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

	auto dlg = GUIHelper::createDialog(widget, "Small variants search");
	GlobalServiceProvider::addModelessDialog(dlg);
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
		QString url = "https://www.ensembl.org/Homo_sapiens/Transcript/Summary?g=" + ensembl_id;
        QDesktopServices::openUrl(QUrl(url));
    }
    else
    {
		GlobalServiceProvider::openGeneTab(link);
	}
}

void GeneWidget::openGeneTab(QString symbol)
{
	GlobalServiceProvider::openGeneTab(symbol);
}

void GeneWidget::updatePhenotypeSearch()
{
	QString search_text = ui_.pheno_search->text().trimmed();

	QStringList tmp;
	foreach(const QString& line, omim_lines)
	{
		if (line.contains(search_text, Qt::CaseInsensitive)) tmp << line;
	}
	ui_.omim->setText(tmp.join("<br>"));

	tmp.clear();
	foreach(const QString& line, orpha_lines)
	{
		if (line.contains(search_text, Qt::CaseInsensitive)) tmp << line;
	}
	ui_.diseases->setText(tmp.join("<br>"));

	tmp.clear();
	foreach(const QString& line, hpo_lines)
	{
		if (line.contains(search_text, Qt::CaseInsensitive)) tmp << line;
	}
	ui_.hpo->setText(tmp.join("<br>"));
}

void GeneWidget::updateTranscriptsTable(NGSD& db)
{
	//clear
	ui_.transcripts->setRowCount(0);
	const QMap<QByteArray, QByteArrayList>&  matches = NGSHelper::transcriptMatches(GSvarHelper::build());

	//get transcripts
	int gene_id = db.geneId(symbol_);
	TranscriptList transcripts = db.transcripts(gene_id, Transcript::ENSEMBL, false);

	//sort transcripts
	transcripts.sortByRelevance();

	//display
	foreach(const Transcript& transcript, transcripts)
	{
		int row = ui_.transcripts->rowCount();
		ui_.transcripts->setRowCount(row+1);

		QLabel* label = GUIHelper::createLinkLabel("<a href='http://www.ensembl.org/Homo_sapiens/Transcript/Summary?t=" + transcript.name() + "'>" + transcript.nameWithVersion() + "</a>");
		ui_.transcripts->setCellWidget(row, 0, label);

		QString coords = "";
		int reg_count = transcript.regions().count();
		if (reg_count>0)
		{
			coords = transcript.regions()[0].chr().strNormalized(true) + ":" + QString::number(transcript.regions()[0].start()) + "-" + QString::number(transcript.regions()[reg_count-1].end());
		}
		ui_.transcripts->setItem(row, 1, GUIHelper::createTableItem(coords));

		QString bases_exons = QString::number(transcript.regions().baseCount()) + " / " + QString::number(transcript.regions().count());
		ui_.transcripts->setItem(row, 2, GUIHelper::createTableItem(bases_exons));

		QString coding_bases_exons = "";
		if (transcript.isCoding()) coding_bases_exons = QString::number(transcript.codingRegions().baseCount()-3) + " / " + QString::number(transcript.codingRegions().count());
		ui_.transcripts->setItem(row, 3, GUIHelper::createTableItem(coding_bases_exons));

		QString aas = "no protein";
		if (transcript.isCoding()) aas = QString::number(transcript.codingRegions().baseCount()/3-1);
		ui_.transcripts->setItem(row, 4, GUIHelper::createTableItem(aas));

		ui_.transcripts->setItem(row, 5, GUIHelper::createTableItem(Transcript::biotypeToString(transcript.biotype())));

		QStringList ccds;
		QStringList refseq;
		foreach(QByteArray match, matches.value(transcript.name()))
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

		QStringList flags = transcript.flags(false);
		ui_.transcripts->setItem(row, 8, GUIHelper::createTableItem(flags.join(", ")));

	}

	GUIHelper::resizeTableCellWidths(ui_.transcripts);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.transcripts);
	GUIHelper::resizeTableHeight(ui_.transcripts);
}
