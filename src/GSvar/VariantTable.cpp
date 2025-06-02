#include "VariantTable.h"
#include "GUIHelper.h"
#include "Exceptions.h"
#include "GSvarHelper.h"
#include "GUIHelper.h"
#include "LoginManager.h"
#include "GlobalServiceProvider.h"
#include "GeneInfoDBs.h"
#include "GenomeVisualizationWidget.h"
#include "ColumnConfig.h"
#include <QBitArray>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QHeaderView>
#include <QDesktopServices>
#include <QMenu>

VariantTable::VariantTable(QWidget* parent)
	: QTableWidget(parent)
	, registered_actions_()
	, active_phenotypes_()
{
	//make sure the selection is visible when the table looses focus
	QString fg = GUIHelper::colorToQssFormat(palette().color(QPalette::Active, QPalette::HighlightedText));
	QString bg = GUIHelper::colorToQssFormat(palette().color(QPalette::Active, QPalette::Highlight));
	setStyleSheet(QString("QTableWidget:!active { selection-color: %1; selection-background-color: %2; }").arg(fg).arg(bg));
	setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenu(QPoint)));
}

void VariantTable::addCustomContextMenuActions(QList<QAction*> actions)
{
	registered_actions_ = actions;
}

void VariantTable::updateActivePhenotypes(PhenotypeList phenotypes)
{
	active_phenotypes_ = phenotypes;
}

void VariantTable::customContextMenu(QPoint pos)
{
	QMenu menu;
	pos = viewport()->mapToGlobal(pos);

	QList<int> indices = selectedVariantsIndices();

	//special case: 2 variants selected -> show compHet upload to ClinVar
	if (indices.count() == 2)
	{
		//ClinVar search
		bool ngsd_user_logged_in = LoginManager::active();
		QAction* a_clinvar_pub = menu.addAction(QIcon("://Icons/ClinGen.png"), "Publish compound-heterozygote variant in ClinVar");
		QMetaMethod signal = QMetaMethod::fromSignal(&VariantTable::publishToClinvarTriggered);
		a_clinvar_pub->setEnabled(ngsd_user_logged_in && isSignalConnected(signal) && ! Settings::string("clinvar_api_key", true).trimmed().isEmpty());

		//execute menu
		QAction* action = menu.exec(pos);
		if (action == a_clinvar_pub) emit publishToClinvarTriggered(indices.at(0), indices.at(1));
		return;
	}
	else if (indices.count() != 1)
	{
		return;
	}

	//else: standard case: 1 variant selected

	int index = indices[0];

	if (registered_actions_.count() > 0)
	{
		foreach (QAction* action, registered_actions_)
		{
			if (action->text() == "---")
			{
				menu.addSeparator();
			}
			else
			{
				menu.insertAction(0, action);
			}
		}
		menu.addSeparator();
	}

	bool  ngsd_user_logged_in = LoginManager::active();
	const Variant variant = (*variants_)[index];
	int i_gene = variants_->annotationIndexByName("gene", true, true);
	GeneSet genes = GeneSet::createFromText(variant.annotations()[i_gene], ',');
	int i_co_sp = variants_->annotationIndexByName("coding_and_splicing", true, true);
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(i_co_sp);
	const QMap<QByteArray, QByteArrayList>& preferred_transcripts = GSvarHelper::preferredTranscripts();

	QAction* a_cnv_sv = menu.addAction("show CNVs/SVs in gene");

	QAction* a_visualize = menu.addAction("Visualize");
	a_visualize->setEnabled(Settings::boolean("debug_mode_enabled", true));
	menu.addSeparator();

	//Google and Google Scholar
	QMenu* sub_menu = menu.addMenu(QIcon("://Icons/Google.png"), "Google");
	QMenu* sub_menu2 = menu.addMenu(QIcon("://Icons/GoogleScholar.png"), "Google Scholar");
	foreach(const VariantTranscript& trans, transcripts)
	{
		QAction* action = sub_menu->addAction(trans.gene + " " + trans.idWithoutVersion() + " " + trans.hgvs_c + " " + trans.hgvs_p);
		QAction* action2 = sub_menu2->addAction(trans.gene + " " + trans.idWithoutVersion() + " " + trans.hgvs_c + " " + trans.hgvs_p);
		if (preferred_transcripts.value(trans.gene).contains(trans.idWithoutVersion()))
		{
			QFont font = action->font();
			font.setBold(true);
			action->setFont(font);
			action2->setFont(font);
		}
	}

	//Alamut
	if (Settings::contains("alamut_host") && Settings::contains("alamut_institution") && Settings::contains("alamut_apikey"))
	{
		sub_menu = menu.addMenu(QIcon("://Icons/Alamut.png"), "Alamut");

		//BAM
		if (variants_->type()==GERMLINE_SINGLESAMPLE)
		{
			sub_menu->addAction("BAM");
		}

		//genomic location
		QString loc = variant.chr().str() + ":" + QByteArray::number(variant.start());
		loc.replace("chrMT", "chrM");
		sub_menu->addAction(loc);
		sub_menu->addAction(loc + variant.ref() + ">" + variant.obs());

		//genes
        for (const QByteArray& g : genes)
		{
			sub_menu->addAction(g);
		}
		sub_menu->addSeparator();

		//transcripts
        for (const VariantTranscript& transcript : transcripts)
		{
			if  (transcript.id!="" && transcript.hgvs_c!="")
			{
				QAction* action = sub_menu->addAction(transcript.idWithoutVersion() + ":" + transcript.hgvs_c + " (" + transcript.gene + ")");

				//highlight preferred transcripts
				if (preferred_transcripts.value(transcript.gene).contains(transcript.idWithoutVersion()))
				{
					QFont font = action->font();
					font.setBold(true);
					action->setFont(font);
				}
			}
		}
		QMetaMethod signal = QMetaMethod::fromSignal(&VariantTable::alamutTriggered);
		sub_menu->setEnabled(isSignalConnected(signal));
	}

	//UCSC
	QAction* a_ucsc = menu.addAction(QIcon("://Icons/UCSC.png"), "UCSC Genome Browser");
	QAction* a_ucsc_enigma = menu.addAction(QIcon("://Icons/UCSC.png"), "UCSC Genome Browser (ENIGMA tracks)");

	//LOVD
	QAction* a_lovd = menu.addAction(QIcon("://Icons/LOVD.png"), "Find in LOVD");

	//ClinVar search
	sub_menu = menu.addMenu(QIcon("://Icons/ClinGen.png"), "ClinVar");
	QAction* a_clinvar_find = sub_menu->addAction("Find in ClinVar");
	QAction* a_clinvar_pub = sub_menu->addAction("Publish in ClinVar");
	QMetaMethod signal = QMetaMethod::fromSignal(&VariantTable::publishToClinvarTriggered);
	a_clinvar_pub->setEnabled(ngsd_user_logged_in && isSignalConnected(signal) && ! Settings::string("clinvar_api_key", true).trimmed().isEmpty());

	//varsome
	QAction* a_varsome = menu.addAction(QIcon("://Icons/VarSome.png"), "VarSome");

	//PubMed
	sub_menu = menu.addMenu(QIcon("://Icons/PubMed.png"), "PubMed");
	//create links for each gene/disease
    for (const QByteArray& g : genes)
	{
		sub_menu->addAction(g + " AND \"mutation\"");
		sub_menu->addAction(g + " AND \"variant\"");
        for (const Phenotype& p : active_phenotypes_)
		{
			sub_menu->addAction(g + " AND \"" + p.name().trimmed() + "\"");
		}
	}

	QAction* a_heredivar = menu.addAction(QIcon("://Icons/HerediVar.png"), "HerediVar");
	QString heredivar_url = Settings::string("HerediVar", true).trimmed();
	a_heredivar->setEnabled(!heredivar_url.isEmpty());

	//add gene databases
	if (!genes.isEmpty())
	{
		menu.addSeparator();
        for (const QByteArray& g : genes)
		{
			sub_menu = menu.addMenu(g);
			sub_menu->addAction(QIcon("://Icons/NGSD_gene.png"), "Gene tab")->setEnabled(ngsd_user_logged_in);
			sub_menu->addAction(QIcon("://Icons/Google.png"), "Google");
            for (const GeneDB& db : GeneInfoDBs::all())
			{
				sub_menu->addAction(db.icon, db.name);
			}
		}
	}

	//add custom entries
	QString custom_menu_small_variants = Settings::string("custom_menu_small_variants", true).trimmed();
	if (!custom_menu_small_variants.isEmpty())
	{
		sub_menu = menu.addMenu("Custom");
		QStringList custom_entries = custom_menu_small_variants.split("\t");
		foreach(QString custom_entry, custom_entries)
		{
			QStringList parts = custom_entry.split("|");
			if (parts.count()==2)
			{
				sub_menu->addAction(parts[0]);
			}
		}
	}

	//execute menu
	QAction* action = menu.exec(pos);
	if (!action) return;


	//perform actions:
	QByteArray text = action->text().toUtf8();
	QMenu* parent_menu = qobject_cast<QMenu*>(action->parent());


	//perform actions
	if (action==a_cnv_sv)
	{
		emit showMatchingCnvsAndSvs(BedLine(variant.chr(), variant.start(), variant.end()));
	}

	if (action==a_visualize)
	{
		FastaFileIndex genome_idx(Settings::string("reference_genome", false));
		GenomeVisualizationWidget* widget = new GenomeVisualizationWidget(this, genome_idx, NGSD().transcripts());
		widget->setRegion(variant.chr(), variant.start(), variant.end());
		auto dlg = GUIHelper::createDialog(widget, "GSvar Genome Viewer");
		dlg->exec();
	}
	else if (parent_menu && (parent_menu->title()=="Google" || parent_menu->title()=="Google Scholar"))
	{
		QByteArray query;
		QByteArrayList parts = text.split(' ');
		QByteArray gene = parts[0].trimmed();
		QByteArray hgvs_c = parts[2].trimmed();
		QByteArray hgvs_p = parts[3].trimmed();
		query = gene + " AND (\"" + hgvs_c.mid(2) + "\" OR \"" + hgvs_c.mid(2).replace(">", "->") + "\" OR \"" + hgvs_c.mid(2).replace(">", "-->") + "\" OR \"" + hgvs_c.mid(2).replace(">", "/") + "\"";
		if (hgvs_p!="")
		{
			QByteArray protein_change = hgvs_p.mid(2).trimmed();
			query += " OR \"" + protein_change + "\"";
			if (QRegularExpression("^[A-Za-z]{3}[0-9]+[A-Za-z]{3}$").match(protein_change).hasMatch() && !protein_change.endsWith("del"))
			{
				QByteArray aa1 = protein_change.left(3);
				QByteArray aa2 = protein_change.right(3);
				QByteArray pos = protein_change.mid(3, protein_change.length()-6);

				query += QByteArray(" OR \"") + NGSHelper::oneLetterCode(aa1) + pos + NGSHelper::oneLetterCode(aa2) + "\"";
			}
			else if (protein_change.endsWith("=")) //special handling of synonymous variants
			{
				QByteArray aa1 = protein_change.left(3);
				QByteArray rest = protein_change.mid(3);

				query += QByteArray(" OR \"") + NGSHelper::oneLetterCode(aa1) + rest + "\"";
			}
		}

		int i_dbsnp = variants_->annotationIndexByName("dbSNP", true, true);
		QByteArray dbsnp = (*variants_)[index].annotations()[i_dbsnp].trimmed();
		if (dbsnp!="")
		{
			query += " OR \"" + dbsnp + "\"";
		}
		query += ")";

		QString base_url = parent_menu->title()=="Google" ? "https://www.google.com/search?q=" : "https://scholar.google.de/scholar?q=";
		QDesktopServices::openUrl(QUrl(base_url + query.replace("+", "%2B").replace(' ', '+')));
	}
	else if (parent_menu && parent_menu->title()=="Alamut")
	{
		emit alamutTriggered(action);
	}
	else if (action == a_ucsc)
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db="+buildToString(GSvarHelper::build())+"&position=" + variant.chr().str()+":"+QString::number(variant.start()-20)+"-"+QString::number(variant.end()+20)));
	}
	else if (action == a_ucsc_enigma)
	{
		QDesktopServices::openUrl(QUrl("https://genome.ucsc.edu/cgi-bin/hgTracks?db=hg38&position=" + variant.chr().str()+":"+QString::number(variant.start()-20)+"-"+QString::number(variant.end()+20)+"&hgsid=2395490845_kJTSEC0eF5uwLq9urfDpQRoxIEPY"));
	}
	else if (action == a_lovd)
	{
		int pos = variant.start();
		if (variant.ref()=="-") pos += 1;
		QDesktopServices::openUrl(QUrl("https://databases.lovd.nl/shared/variants?search_chromosome=%3D%22" + variant.chr().strNormalized(false) + "%22&search_VariantOnGenome/DNA"+(GSvarHelper::build()==GenomeBuild::HG38 ? "/hg38" : "")+"=g." + QString::number(pos)));
	}
	else if (action == a_clinvar_find)
	{
		QString url = GSvarHelper::clinVarSearchLink(variant, GSvarHelper::build());
		QDesktopServices::openUrl(QUrl(url));
	}
	else if (action == a_clinvar_pub)
	{
		emit publishToClinvarTriggered(index);
	}
	else if (action == a_varsome)
	{
		QString ref = variant.ref();
		ref.replace("-", "");
		QString obs = variant.obs();
		obs.replace("-", "");
		QString var = variant.chr().str() + "-" + QString::number(variant.start()) + "-" +  ref + "-" + obs;
		QString genome = variant.chr().isM() ? "hg38" : buildToString(GSvarHelper::build());
		QDesktopServices::openUrl(QUrl("https://varsome.com/variant/" + genome + "/" + var));
	}
	else if (parent_menu && parent_menu->title()=="PubMed")
	{
		QDesktopServices::openUrl(QUrl("https://pubmed.ncbi.nlm.nih.gov/?term=" + text));
	}
	else if (action == a_heredivar)
	{
		FastaFileIndex genome_idx(Settings::string("reference_genome", false));
		VcfLine vcf = variant.toVCF(genome_idx);
		QDesktopServices::openUrl(QUrl(heredivar_url + "/display/chr=" + vcf.chr().str() + "&pos=" + QString::number(vcf.start()) + "&ref=" + vcf.ref() + "&alt=" + vcf.altString()));
	}
	else if (parent_menu && genes.contains(parent_menu->title().toUtf8())) //gene menus
	{
		QString gene = parent_menu->title();

		if (text=="Gene tab")
		{
			GlobalServiceProvider::openGeneTab(gene);
		}
		else if (text=="Google")
		{
			QString query = gene + " AND (mutation";
            for (const Phenotype& pheno : active_phenotypes_)
			{
				query += " OR \"" + pheno.name() + "\"";
			}
			query += ")";

			QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + query.replace("+", "%2B").replace(' ', '+')));
		}
		else //other databases
		{
			GeneInfoDBs::openUrl(text, gene);
		}
	}
	else if (parent_menu && parent_menu->title()=="Custom")
	{
		QStringList custom_entries = Settings::string("custom_menu_small_variants", true).trimmed().split("\t");
        for (QString custom_entry : custom_entries)
		{
			QStringList parts = custom_entry.split("|");
			if (parts.count()==2 && parts[0]==text)
			{
				QString url = parts[1];
				url.replace("[chr]", variant.chr().strNormalized(true));
				url.replace("[start]", QString::number(variant.start()));
				url.replace("[end]", QString::number(variant.end()));
				url.replace("[ref]", variant.ref());
				url.replace("[obs]", variant.obs());
				QDesktopServices::openUrl(QUrl(url));
			}
		}
	}
	else
	{
		// emit signal for added actions
		emit customActionTriggered(action, index);
	}
}

void VariantTable::updateTable(VariantList& variants, const FilterResult& filter_result, const QHash<int,bool>& index_show_report_icon, const QSet<int>& index_causal, int max_variants)
{
	//update local reference to the variants
	variants_ = &variants;

	//set rows and cols
	int row_count_new = std::min(filter_result.countPassing(), max_variants);
	int col_count_new = 5 + variants.annotations().count();
	if (rowCount()!=row_count_new || columnCount()!=col_count_new)
	{
		//completely clear items (is faster than resizing)
		clearContents();
		//set new size
		setRowCount(row_count_new);
		setColumnCount(col_count_new);
	}

	//header
	setHorizontalHeaderItem(0, createTableItem("chr"));
	horizontalHeaderItem(0)->setToolTip("Chromosome the variant is located on.");
	setHorizontalHeaderItem(1, createTableItem("start"));
	horizontalHeaderItem(1)->setToolTip("Start position of the variant on the chromosome.\nFor insertions, the position of the base before the insertion is shown.");
	setHorizontalHeaderItem(2, createTableItem("end"));
	horizontalHeaderItem(2)->setToolTip("End position of the variant on the chromosome.\nFor insertions, the position of the base before the insertion is shown.");
	setHorizontalHeaderItem(3, createTableItem("ref"));
	horizontalHeaderItem(3)->setToolTip("Reference bases in the reference genome at the variant position.\n`-` in case of an insertion.");
	setHorizontalHeaderItem(4, createTableItem("obs"));
	horizontalHeaderItem(4)->setToolTip("Alternate bases observed in the sample.\n`-` in case of an deletion.");

	//add columns
	ColumnConfig config = ColumnConfig::fromString(Settings::string("column_config_small_variant", true));
	QStringList col_order;
	QList<int> anno_index_order;
	config.getOrder(variants, col_order, anno_index_order);
	for (int i=0; i<col_order.count(); ++i)
	{
		QString anno = col_order[i];
		QTableWidgetItem* header = new QTableWidgetItem(anno);

		//additional descriptions for filter column
		QString add_desc = "";
		if (anno=="filter")
		{
			auto it = variants.filters().cbegin();
			while (it!=variants.filters().cend())
			{
				add_desc += "\n - "+it.key() + ": " + it.value();
				++it;
			}
		}

		//additional descriptions and color for genotype columns
		SampleHeaderInfo sample_data = variants.getSampleHeader();
		foreach(const SampleInfo& info, sample_data)
		{
			if (info.name==anno)
			{
				auto it = info.properties.cbegin();
				while(it != info.properties.cend())
				{
					add_desc += "\n - " + it.key() + ": " + it.value();

					if (info.isAffected())
					{
						header->setForeground(QBrush(Qt::darkRed));
					}

					++it;
				}
			}
		}

		QString header_desc = variants.annotationDescriptionByName(anno, false).description();
		header->setToolTip(header_desc + add_desc);
		setHorizontalHeaderItem(i+5, header);
	}

	//content
	int i_genes = variants.annotationIndexByName("gene", true, false);
	int i_co_sp = variants.annotationIndexByName("coding_and_splicing", true, false);
	int i_validation = variants.annotationIndexByName("validation", true, false);
	int i_classification = variants.annotationIndexByName("classification", true, false);
	int i_comment = variants.annotationIndexByName("comment", true, false);
	int i_ihdb_hom = variants.annotationIndexByName("NGSD_hom", true, false);
	int i_ihdb_het = variants.annotationIndexByName("NGSD_het", true, false);
	int i_clinvar = variants.annotationIndexByName("ClinVar", true, false);
	int i_hgmd = variants.annotationIndexByName("HGMD", true, false);
	int i_spliceai = variants.annotationIndexByName("SpliceAI", true, false);
	int i_maxentscan = variants.annotationIndexByName("MaxEntScan", true, false);
	int r = -1;
	for (int i=0; i<variants.count(); ++i)
	{
		if (!filter_result.passing(i)) continue;

		++r;
		if (r>=max_variants) break; //maximum number of variants reached > abort
		const Variant& variant = variants[i];

		setItem(r, 0, createTableItem(variant.chr().str()));
		if (!variant.chr().isAutosome())
		{
            item(r,0)->setBackground(QBrush(QColor(Qt::yellow)));
			item(r,0)->setToolTip("Not autosome");
		}
		setItem(r, 1, createTableItem(QByteArray::number(variant.start())));
		setItem(r, 2, createTableItem(QByteArray::number(variant.end())));
		setItem(r, 3, createTableItem(variant.ref()));
		setItem(r, 4, createTableItem(variant.obs()));
		bool is_warning_line = false;
		bool is_notice_line = false;
		bool is_ngsd_benign = false;
		for (int j=0; j<variant.annotations().count(); ++j)
		{
			int anno_index = anno_index_order[j];
			const QByteArray& anno = variant.annotations().at(anno_index);
			QTableWidgetItem* item = createTableItem(anno);

			//warning
			if (anno_index==i_co_sp && anno.contains(":HIGH:"))
			{
                item->setBackground(QBrush(QColor(Qt::red)));
				is_warning_line = true;
			}
			else if (anno_index==i_classification && (anno=="3" || anno=="M" || anno=="R"))
			{
                item->setBackground(QBrush(QColor(QColor(255, 135, 60)))); //orange
				is_notice_line = true;
			}
			else if (anno_index==i_classification && (anno=="4" || anno=="5"))
			{
                item->setBackground(QBrush(QColor(Qt::red)));
				is_warning_line = true;
			}
			else if (anno_index==i_clinvar && anno.contains("pathogenic") && !anno.contains("conflicting interpretations of pathogenicity")) //matches "pathogenic" and "likely pathogenic"
			{
                item->setBackground(QBrush(QColor(Qt::red)));
				is_warning_line = true;
			}
			else if (anno_index==i_hgmd && anno.contains("CLASS=DM")) //matches both "DM" and "DM?"
			{
                item->setBackground(QBrush(QColor(Qt::red)));
				is_warning_line = true;
			}
			else if (anno_index==i_spliceai && NGSHelper::maxSpliceAiScore(anno) >= 0.8)
			{
                item->setBackground(QBrush(QColor(Qt::red)));
				is_notice_line = true;
			}
			else if (anno_index==i_spliceai && NGSHelper::maxSpliceAiScore(anno) >= 0.5)
			{
                item->setBackground(QBrush(QColor(QColor(255, 135, 60)))); //orange
				is_notice_line = true;
			}
			else if (anno_index==i_maxentscan && !anno.isEmpty())
			{
				//iterate over predictions per transcript
				QList<MaxEntScanImpact> impacts;
				foreach(const QByteArray& entry, anno.split(','))
				{
					QByteArray anno_with_percentages;
					try
					{
						impacts << NGSHelper::maxEntScanImpact(entry.split('/'), anno_with_percentages, false);
					}
					catch (Exception& e) //catch error of outdated MaxEntScan annotation
					{
						qDebug() << e.message();
					}
				}

				//output: max import
				if (impacts.contains(MaxEntScanImpact::HIGH))
				{
                    item->setBackground(QBrush(QColor(Qt::red))); //orange
					is_notice_line = true;
				}
				else if (impacts.contains(MaxEntScanImpact::MODERATE))
				{
                    item->setBackground(QBrush(QColor(QColor(255, 135, 60)))); //orange
					is_notice_line = true;
				}
			}

			//non-pathogenic
			if (anno_index==i_classification && (anno=="1" || anno=="2"))
			{
                item->setBackground(QBrush(QColor(Qt::green)));
				is_ngsd_benign = true;
			}

			//highlighed
			if (anno_index==i_validation && anno.contains("TP"))
			{
                item->setBackground(QBrush(QColor(Qt::yellow)));
			}
			else if (anno_index==i_comment && anno!="")
			{
                item->setBackground(QBrush(QColor(Qt::yellow)));
			}
			else if (anno_index==i_ihdb_hom && anno=="0")
			{
                item->setBackground(QBrush(QColor(Qt::yellow)));
			}
			else if (anno_index==i_ihdb_het && anno=="0")
			{
                item->setBackground(QBrush(QColor(Qt::yellow)));
			}
			else if (anno_index==i_clinvar && anno.contains("(confirmed)"))
			{
                item->setBackground(QBrush(QColor(Qt::yellow)));
			}
			else if (anno_index==i_genes)
			{
				GeneSet anno_genes = GeneSet::createFromText(anno, ',');
				GSvarHelper::colorGeneItem(item, anno_genes);
			}

			setItem(r, 5+j, item);
		}

		//vertical headers - warning (red), notice (orange)
		QTableWidgetItem* item = createTableItem(QByteArray::number(i+1));
		item->setData(Qt::UserRole, i); //store variant index in user data (for selection methods)
		if (!is_ngsd_benign)
		{
			if (is_warning_line)
			{
				item->setForeground(QBrush(Qt::red));
				QFont font;
				font.setWeight(QFont::Bold);
				item->setFont(font);
			}
			else if (is_notice_line)
			{
				item->setForeground(QBrush(QColor(255, 135, 60)));
				QFont font;
				font.setWeight(QFont::Bold);
				item->setFont(font);
			}
		}
		if (index_show_report_icon.contains(i))
		{
			item->setIcon(reportIcon(index_show_report_icon.value(i), index_causal.contains(i)));
		}
		setVerticalHeaderItem(r, item);
	}

	//hide columns if requested
	config.applyHidden(this);
}

void VariantTable::update(VariantList& variants, const FilterResult& filter_result, const ReportSettings& report_settings, int max_variants)
{
	//init
	QHash<int, bool> index_show_report_icon;
	QSet<int> index_causal;
	foreach(int index, report_settings.report_config->variantIndices(VariantType::SNVS_INDELS, false))
	{
		const ReportVariantConfiguration& rc = report_settings.report_config->get(VariantType::SNVS_INDELS, index);
		index_show_report_icon[index] = rc.showInReport();
		if (rc.causal) index_causal << index;
	}

	updateTable(variants, filter_result, index_show_report_icon, index_causal, max_variants);
}

void VariantTable::update(VariantList& variants, const FilterResult& filter_result, const SomaticReportSettings& report_settings, int max_variants)
{
	//init
	QHash<int, bool> index_show_report_icon;
	QSet<int> index_causal;
	foreach(int index, report_settings.report_config->variantIndices(VariantType::SNVS_INDELS, false))
	{
		index_show_report_icon[index] = report_settings.report_config->get(VariantType::SNVS_INDELS, index).showInReport();
	}

	updateTable(variants, filter_result, index_show_report_icon, index_causal, max_variants);
}

void VariantTable::updateVariantHeaderIcon(const ReportSettings& report_settings, int variant_index)
{
	int row = variantIndexToRow(variant_index);

	QIcon report_icon;
	if (report_settings.report_config->exists(VariantType::SNVS_INDELS, variant_index))
	{
		const ReportVariantConfiguration& rc = report_settings.report_config->get(VariantType::SNVS_INDELS, variant_index);
		report_icon = reportIcon(rc.showInReport(), rc.causal);
	}
	verticalHeaderItem(row)->setIcon(report_icon);
}

void VariantTable::updateVariantHeaderIcon(const SomaticReportSettings &report_settings, int variant_index)
{
	int row = variantIndexToRow(variant_index);
	QIcon report_icon;
	if(report_settings.report_config->exists(VariantType::SNVS_INDELS, variant_index))
	{
		report_icon = reportIcon(report_settings.report_config->get(VariantType::SNVS_INDELS, variant_index).showInReport(), false);
	}
	verticalHeaderItem(row)->setIcon(report_icon);
}

int VariantTable::selectedVariantIndex(bool gui_indices) const
{
	QList<int> indices = selectedVariantsIndices(gui_indices);

	if (indices.count()!=1) return -1;

	return indices[0];
}

QList<int> VariantTable::selectedVariantsIndices(bool gui_indices) const
{
	QList<int> output;

	QList<QTableWidgetSelectionRange> ranges = selectedRanges();
	foreach(const QTableWidgetSelectionRange& range, ranges)
	{
		for(int row=range.topRow(); row<=range.bottomRow(); ++row)
		{
			if (gui_indices)
			{
				output << row;
			}
			else
			{
				output << rowToVariantIndex(row);
			}
		}
	}

	std::sort(output.begin(), output.end());

	return output;
}

int VariantTable::rowToVariantIndex(int row) const
{
	//get header (variant index is stored in user data)
	QTableWidgetItem* header = verticalHeaderItem(row);
	if (header==nullptr) THROW(ProgrammingException, "Variant table row header not set!");

	//convert header text to integer
	bool ok;
	int variant_index = header->data(Qt::UserRole).toInt(&ok);
	if (!ok) THROW(ProgrammingException, "Variant table row header user data '" + header->data(Qt::UserRole).toString() + "' is not an integer!");

	return variant_index;
}

int VariantTable::variantIndexToRow(int index) const
{
	for (int row=0; row<verticalHeader()->count(); ++row)
	{
		QTableWidgetItem* item = verticalHeaderItem(row);
		if (item==nullptr) THROW(ProgrammingException, "Variant table row header not set for row '" + QString::number(row) + "'!");

		bool ok;
		int index_header = item->data(Qt::UserRole).toInt(&ok);
		if (!ok) THROW(ProgrammingException, "Variant table row header user data '" + item->data(Qt::UserRole).toString() + "' is not an integer!");

		if(index_header==index)
		{
			return row;
		}
	}

	THROW(ProgrammingException, "Variant table row header not found for variant with index '" + QString::number(index) + "'!");
}

QList<int> VariantTable::columnWidths() const
{
	QList<int> output;

	for (int c=0; c<columnCount(); ++c)
	{
		output << columnWidth(c);
	}

	return output;
}

void VariantTable::setColumnWidths(const QList<int>& widths)
{
    int col_count = std::min(SIZE_TO_INT(widths.count()), SIZE_TO_INT(columnCount()));
	for (int c=0; c<col_count; ++c)
	{
		setColumnWidth(c, widths[c]);
	}
}

void VariantTable::adaptRowHeights()
{
	if (rowCount()==0) return;

	//determine minimum height of first 10 rows
	resizeRowToContents(0);
	int height = rowHeight(0);
	for (int i=1; i<std::min(10, rowCount()); ++i)
	{
		resizeRowToContents(i);
		if (rowHeight(i)<height) height = rowHeight(i);
	}

	//set height for all columns
	for (int i=0; i<rowCount(); ++i)
	{
		setRowHeight(i, height);
	}
}

void VariantTable::clearContents()
{
	setRowCount(0);
	setColumnCount(0);
}

void VariantTable::adaptColumnWidths()
{
    QElapsedTimer timer;
	timer.start();

	//restrict width
	ColumnConfig config = ColumnConfig::fromString(Settings::string("column_config_small_variant", true));
	config.applyColumnWidths(this);

	//set mimumn width of chr, start, end
	if (columnWidth(0)<42)
	{
		setColumnWidth(0, 42);
	}
	if (columnWidth(1)<62)
	{
		setColumnWidth(1, 62);
	}
	if (columnWidth(2)<62)
	{
		setColumnWidth(2, 62);
	}

	//restrict REF/ALT column width
	for (int i=3; i<=4; ++i)
	{
		if (columnWidth(i)>50)
		{
			setColumnWidth(i, 50);
		}
	}

	Log::perf("adapting column widths", timer);
}

void VariantTable::showAllColumns()
{
	for (int c=0; c<columnCount(); ++c)
	{
		if(isColumnHidden(c))
		{
			setColumnHidden(c, false);

			//make sure hidden columns have a non-zero width
			if (c>5 && columnWidth(c)==0)
			{
				setColumnWidth(c, 200);
			}
		}
	}
}

void VariantTable::copyToClipboard(bool split_quality, bool include_header_one_row)
{
	// Data to be copied is not selected en bloc
	if (selectedRanges().count()!=1 && !split_quality)
	{
		//Create 2d list with empty QStrings (size equal to QTable in Main Window)
		QList< QList<QString> > data;
		for(int r=0;r<rowCount();++r)
		{
			QList<QString> line;
			for(int c=0;c<columnCount();++c)
			{
				line.append("");
			}
			data.append(line);
		}

		//Fill data with non-empty entries from QTable in Main Window
		QBitArray empty_columns;
		empty_columns.fill(true,columnCount());
		QList<QTableWidgetItem*> all_items = selectedItems();
		foreach(QTableWidgetItem* item,all_items)
		{
			if(!item->text().isEmpty())
			{
				data[item->row()][item->column()] = item->text();
				empty_columns[item->column()] = false;
			}
		}

		//Remove empty columns
		for(int c=columnCount()-1;c>=0;--c)
		{
			if(empty_columns[c])
			{
				for(int r=0;r<rowCount();++r)
				{
					data[r].removeAt(c);
				}
			}
		}

		//Remove empty rows
		for(int r=rowCount()-1;r>=0;--r)
		{
			bool row_is_empty = true;
			for(int c=0;c<data[r].count();++c)
			{
				if(!data[r][c].isEmpty())
				{
					row_is_empty = false;
					break;
				}
			}
			if(row_is_empty) data.removeAt(r);
		}

		QString text = "";
		for(int r=0;r<data.count();++r)
		{
			for(int c=0;c<data[r].count();++c)
			{
				text.append(data[r][c]);
				if(c<data[r].count()-1) text.append("\t");
			}
			text.append("\n");
		}
		QApplication::clipboard()->setText(text);

		return;
	}

	QTableWidgetSelectionRange range = selectedRanges()[0];

	//check quality column is present
	QStringList quality_keys;
	quality_keys << "QUAL" << "DP" << "QD" << "AF" << "MQM" << "SAP" << "ABP" << "TRIO" << "SAR" << "SAF"; //if modified, also modify quality_values!!!
	int qual_index = -1;
	if (split_quality)
	{
		qual_index =  GUIHelper::columnIndex(this, "quality", false);
		if (qual_index==-1)
		{
			QMessageBox::warning(this, "Copy to clipboard", "Column with index 6 has other name than quality. Aborting!");
			return;
		}
	}


	//copy header
	QString selected_text = "";
	if (range.rowCount()!=1 || include_header_one_row)
	{
		selected_text += "#";
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");
			if (split_quality && col==qual_index)
			{
				selected_text.append(quality_keys.join('\t'));
			}
			else
			{
				selected_text.append(horizontalHeaderItem(col)->text());
			}
		}
	}

	//copy rows
	for (int row=range.topRow(); row<=range.bottomRow(); ++row)
	{
		if (selected_text!="") selected_text.append("\n");
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");

			QTableWidgetItem* current_item = item(row, col);
			if (current_item==nullptr) continue;

			if (split_quality && col==qual_index)
			{
				QStringList quality_values;
				for(int i=0; i<quality_keys.count(); ++i) quality_values.append("");
				QStringList entries = current_item->text().split(';');
				foreach(const QString& entry, entries)
				{
					QStringList key_value = entry.split('=');
					if (key_value.count()!=2)
					{
						QMessageBox::warning(this, "Copy to clipboard", "Cannot split quality entry '" + entry + "' into key and value. Aborting!");
						return;
					}
					int index = quality_keys.indexOf(key_value[0]);
					if (index==-1)
					{
						QMessageBox::warning(this, "Copy to clipboard", "Unknown quality entry '" + key_value[0] + "'. Aborting!");
						return;
					}

					quality_values[index] = key_value[1];
				}
				selected_text.append(quality_values.join('\t'));
			}
			else
			{
				selected_text.append(current_item->text().replace('\n',' ').replace('\r', ""));
			}
		}
	}

	QApplication::clipboard()->setText(selected_text);
}

QIcon VariantTable::reportIcon(bool show_in_report, bool causal)
{
	if (!show_in_report) return QPixmap(":/Icons/Report_exclude.png");
	if (causal) return QPixmap(":/Icons/Report_add_causal.png");

	return QPixmap(":/Icons/Report_add.png");
}

void VariantTable::keyPressEvent(QKeyEvent* event)
{
	if(event->matches(QKeySequence::Copy))
	{
		copyToClipboard();
	}
	else if(event->key()==Qt::Key_C && event->modifiers() == (Qt::ShiftModifier|Qt::ControlModifier))
	{
		copyToClipboard(true);
	}
	else if(event->key()==Qt::Key_C && event->modifiers() == (Qt::AltModifier|Qt::ControlModifier))
	{
		copyToClipboard(false, true);
	}
	else //default key-press event
	{
		QTableWidget::keyPressEvent(event);
	}
}
