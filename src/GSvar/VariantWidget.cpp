#include "VariantWidget.h"
#include "ClassificationDialog.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "DBTableWidget.h"
#include "GSvarHelper.h"
#include "GlobalServiceProvider.h"
#include <QDialog>
#include <QMessageBox>
#include <QAction>
#include <QDesktopServices>
#include <QInputDialog>
#include <QClipboard>
#include "VariantHgvsAnnotator.h"
#include "ClientHelper.h"
#include "Background/VariantAnnotator.h"

VariantWidget::VariantWidget(const Variant& variant, QWidget *parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
	, variant_(variant)
{
	ui_.setupUi(this);
	connect(ui_.similarity, SIGNAL(clicked(bool)), this, SLOT(calculateSimilarity()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_.edit_btn, SIGNAL(clicked(bool)), this, SLOT(editComment()));
	connect(ui_.class_btn, SIGNAL(clicked(bool)), this, SLOT(editClassification()));
	connect(ui_.af_gnomad, SIGNAL(linkActivated(QString)), this, SLOT(gnomadClicked(QString)));
	connect(ui_.pubmed, SIGNAL(linkActivated(QString)), this, SLOT(pubmedClicked(QString)));
	connect(ui_.table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(tableCellDoubleClicked(int, int)));
	connect(ui_.anno_btn, SIGNAL(clicked(bool)), this, SLOT(openVariantInTab()));
	ui_.anno_btn->setEnabled(ClientHelper::isClientServerMode());

	//set up copy button for different formats
	QMenu* menu = new QMenu();
	menu->addAction("GSvar", this, SLOT(copyVariant()));
	menu->addAction("VCF", this, SLOT(copyVariant()));
	menu->addAction("HGVS.c", this, SLOT(copyVariant()));
	menu->addAction("gnomAD", this, SLOT(copyVariant()));
	ui_.format_btn->setMenu(menu);

	//add sample table context menu entries
	QAction* action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openProcessedSampleTabs()));

	action = new QAction(QIcon(":/Icons/Icon.png"), "Open variant list", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openGSvarFile()));
}

void VariantWidget::updateGUI()
{
	//get variant id
	NGSD db;
	QString variant_id = db.variantId(variant_);

	//variant base info
	ui_.variant->setText(variant_.toString(QChar(), -1, true));

	SqlQuery query1 = db.getQuery();
	query1.exec("SELECT * FROM variant WHERE id=" + variant_id);
	query1.next();
	QString gnomad_af;
	if (!query1.value("gnomad").isNull())
	{
		gnomad_af = QString::number(query1.value("gnomad").toDouble(), 'f', 5);
	}
	ui_.af_gnomad->setText("<a style=\"color: #000000;\" href=\"" + variant_id + "\">" + gnomad_af + "</a>");

	QVariant cadd = query1.value("cadd");
	ui_.cadd->setText(cadd.isNull() ? "" : cadd.toString());
	QVariant spliceai = query1.value("spliceai");
	ui_.spliceai->setText(spliceai.isNull() ? "" : spliceai.toString());

	//PubMed ids
	QStringList pubmed_ids = db.pubmedIds(variant_id);
	QStringList pubmed_links;
	foreach (const QString& id, pubmed_ids)
	{
		pubmed_links << "<a href=\"https://pubmed.ncbi.nlm.nih.gov/" + id + "/" "\">" + id + "</a>";
	}

	QString open_all;
	if (pubmed_ids.size() > 2)
	{
		open_all = " <a href=\"openAll\"><i>[open all]</i></a>";
	}
	ui_.pubmed->setText(pubmed_links.join(", ") + open_all);
	ui_.pubmed->setToolTip(pubmed_ids.join(", "));

	//annotate consequence for each transcript
	TranscriptList transcripts  = db.transcriptsOverlapping(variant_.chr(), variant_.start(), variant_.end(), 5000);
	transcripts.sortByRelevance();
	FastaFileIndex genome_idx(Settings::string("reference_genome"));
	VariantHgvsAnnotator hgvs_annotator(genome_idx);
	ui_.transcripts->setRowCount(transcripts.count());
	for(int i=0; i<transcripts.count(); ++i)
	{
		const Transcript& trans = transcripts[i];
		VariantConsequence consequence = hgvs_annotator.annotate(trans, variant_);

		QLabel* label = GUIHelper::createLinkLabel("<a href=\"" + trans.gene() + "\">" + trans.gene() + "</a>", false);
		connect(label, SIGNAL(linkActivated(QString)), this, SLOT(openGeneTab(QString)));
		ui_.transcripts->setCellWidget(i, 0, label);
		ui_.transcripts->setItem(i, 1, GUIHelper::createTableItem(trans.nameWithVersion()));
		ui_.transcripts->setItem(i, 2, GUIHelper::createTableItem(consequence.hgvs_c));
		ui_.transcripts->setItem(i, 3, GUIHelper::createTableItem(consequence.hgvs_p));
		ui_.transcripts->setItem(i, 4, GUIHelper::createTableItem(consequence.typesToString(", ")));
		ui_.transcripts->setItem(i, 5, GUIHelper::createTableItem(trans.flags(false).join(", ")));
	}
	GUIHelper::resizeTableCellWidths(ui_.transcripts, 450);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.transcripts);

	//ClinVar
	SqlQuery query4 = db.getQuery();
	query4.exec("SELECT s.name as s_name, vp.db, vp.class, vp.date, vp.result, u.name as u_name FROM sample s, variant_publication vp, user u WHERE s.id=vp.sample_id AND vp.user_id=u.id AND variant_id=" + variant_id + " AND vp.replaced=0 ORDER BY vp.date DESC");
	if (query4.next())
	{
		QString db = query4.value("db").toString();
		if (db=="ClinVar")
		{
			QString url = GSvarHelper::clinVarSearchLink(variant_, GSvarHelper::build());
			db = "<a href=\"" + url + "\">" + db + "</a>";
		}

		ui_.publication_class->setText(query4.value("class").toString() + " " + "(Uploaded to " + db + " by "+ query4.value("u_name").toString() +" on "+ query4.value("date").toString().replace("T", " ") +")");
		ui_.publication_sample->setText(query4.value("s_name").toString());
		ui_.publication_status->setText(query4.value("result").toString());
	}

	//classification
	ClassificationInfo class_info = db.getClassification(variant_);
	ui_.classification->setText(class_info.classification);
	GSvarHelper::limitLines(ui_.classification_comment, class_info.comments);

	//update GUI (next code block is slow)
	qApp->processEvents();

	//NGSD counts
	GenotypeCounts counts = db.genotypeCounts(variant_id);
	QString text;
	text += QString::number(counts.hom)+"x hom, ";
	text += QString::number(counts.het)+"x het, ";
	text += QString::number(counts.mosaic)+"x mosaic";
	ui_.ngsd_counts->setText(text);
	GSvarHelper::limitLines(ui_.comments, query1.value("comment").toString());

	//update GUI (next code block is slow)
	qApp->processEvents();

	//samples table
	SqlQuery query2 = db.getQuery();
	query2.exec("SELECT processed_sample_id, genotype, mosaic FROM detected_variant WHERE variant_id=" + variant_id);
	bool fill_table = true;
	if (query2.size()>250)
	{
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        int res = QMessageBox::question(this, "Many variants detected.", "The variant is in NGSD " + QString::number(query2.size()) + " times.\nShowing the variant table might be slow.\nDo you want to fill the variant table?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        #else
        int res = QMessageBox::question(this, "Many variants detected.", "The variant is in NGSD " + QString::number(query2.size()) + " times.\nShowing the variant table might be slow.\nDo you want to fill the variant table?", QMessageBox::Yes, QMessageBox::No|QMessageBox::Default);
        #endif
		if (res!=QMessageBox::Yes) fill_table = false;
	}

	if (fill_table)
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//resize table
		ui_.table->setRowCount(query2.size());

		//fill samples table
		int row = 0;
		while(query2.next())
		{
			QString ps_id = query2.value(0).toString();
			QString s_id  = db.getValue("SELECT sample_id FROM processed_sample WHERE id=" + ps_id).toString();
			SampleData s_data = db.getSampleData(s_id);
			ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
			DiagnosticStatusData diag_data = db.getDiagnosticStatus(ps_id);
			QTableWidgetItem* item = addItem(row, 0,  ps_data.name);
			DBTableWidget::styleQuality(item, ps_data.quality);
			addItem(row, 1,  s_data.name_external);
			addItem(row, 2,  s_data.patient_identifier);
			addItem(row, 3,  s_data.gender);
			addItem(row, 4,  ps_data.ancestry);
			addItem(row, 5,  s_data.quality + " / " + ps_data.quality);
			QString genotype = query2.value(1).toString();
			if (query2.value(2).toInt()==1) genotype += " (mosaic)";
			addItem(row, 6, genotype);
			addItem(row, 7, ps_data.processing_system);
			addItem(row, 8, ps_data.project_name);
			addItem(row, 9, s_data.disease_group);
			addItem(row, 10, s_data.disease_status);
			QStringList pho_list;
            for (const Phenotype& pheno : s_data.phenotypes)
			{
				pho_list << pheno.toString();
			}
			addItem(row, 11, pho_list.join("; "));
			addItem(row, 12, diag_data.dagnostic_status);
			addItem(row, 13, diag_data.user);
			addItem(row, 14, s_data.comments, true);
			addItem(row, 15, ps_data.comments, true);

			//get causal genes from report config
			GeneSet genes_causal;
			SqlQuery query3 = db.getQuery();
			query3.exec("SELECT v.chr, v.start, v.end FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='diagnostic variant' AND rcv.causal=1 AND rc.processed_sample_id=" + ps_id);
			while(query3.next())
			{
				genes_causal << db.genesOverlapping(query3.value(0).toByteArray(), query3.value(1).toInt(), query3.value(2).toInt(), 5000);
			}
			addItem(row, 16, genes_causal.join(','));

			//get candidate genes from report config
			GeneSet genes_candidate;
			query3.exec("SELECT v.chr, v.start, v.end FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='candidate variant' AND rc.processed_sample_id=" + ps_id);
			while(query3.next())
			{
				genes_candidate << db.genesOverlapping(query3.value(0).toByteArray(), query3.value(1).toInt(), query3.value(2).toInt(), 5000);
			}
			addItem(row, 17, genes_candidate.join(','));

			//add report config comment of variant
			QString rc_comment;
			query3.exec("SELECT CONCAT(rcv.comments, ' // ', rcv.comments2) FROM report_configuration rc, report_configuration_variant rcv WHERE rcv.report_configuration_id=rc.id AND rc.processed_sample_id=" + ps_id + " AND rcv.variant_id=" + variant_id);
			if(query3.next())
			{
				rc_comment = query3.value(0).toString().trimmed();
			}
			addItem(row, 18, rc_comment, true);

			//validation info
			QString vv_id = db.getValue("SELECT id FROM variant_validation WHERE sample_id='" + s_id + "' AND variant_id='" + variant_id + "' AND variant_type='SNV_INDEL'").toString();
			if (!vv_id.isEmpty())
			{
				addItem(row, 19, db.getValue("SELECT status FROM variant_validation WHERE id='" + vv_id + "'").toString());
			}

			++row;
		}

		//sort by processed sample name
		ui_.table->sortByColumn(0, ui_.table->horizontalHeader()->sortIndicatorOrder());

		//resize table cols
		GUIHelper::resizeTableCellWidths(ui_.table, 200);
		GUIHelper::resizeTableCellHeightsToFirst(ui_.table);

		QApplication::restoreOverrideCursor();
	}
}

void VariantWidget::delayedInitialization()
{
	updateGUI();
}


QTableWidgetItem* VariantWidget::addItem(int r, int c, QString text, bool also_as_tooltip)
{
	if (c>=ui_.table->columnCount())
	{
		THROW(ProgrammingException, "Column '" + QString::number(c) + "' not present in variant table!");
	}

	QTableWidgetItem* item = new QTableWidgetItem(text);
	ui_.table->setItem(r, c, item);
	if (also_as_tooltip)
	{
		ui_.table->item(r, c)->setToolTip(text);
	}
	return item;
}

void VariantWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.table);
}

void VariantWidget::calculateSimilarity()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;

	//get sample names and variant lists
	QStringList ps_names;
	QList<QSet<QString>> ps_vars;
	for(int i=0; i<ui_.table->rowCount(); ++i)
	{
		QString ps = ui_.table->item(i, 0)->text();
		ps_names << ps;

		QString ps_id = db.processedSampleId(ps);
        ps_vars << LIST_TO_SET(db.getValues("SELECT variant_id FROM detected_variant WHERE processed_sample_id=" + ps_id + " AND mosaic=0"));
	}

	//calculate and show overlap
	QTableWidget* table = new QTableWidget(this);
	table->setMinimumSize(700, 550);
	table->setEditTriggers(QTableWidget::NoEditTriggers);
	table->setColumnCount(6);
	table->setHorizontalHeaderLabels(QStringList() << "s1" << "#variants s1" << "s2" << "#variants s2" << "variant overlap" << "variant overlap %");
	int row = 0;
	for (int i=0; i<ps_vars.count(); ++i)
	{
		for (int j=i+1; j<ps_vars.count(); ++j)
		{
			table->setRowCount(table->rowCount()+1);
			table->setItem(row, 0, new QTableWidgetItem(ps_names[i]));
			int c_s1 = ps_vars[i].count();
			table->setItem(row, 1, new QTableWidgetItem(QString::number(c_s1)));
			table->setItem(row, 2, new QTableWidgetItem(ps_names[j]));
			int c_s2 = ps_vars[j].count();
			table->setItem(row, 3, new QTableWidgetItem(QString::number(c_s2)));
			int overlap = QSet<QString>(ps_vars[i]).intersect(ps_vars[j]).count();
			table->setItem(row, 4, new QTableWidgetItem(QString::number(overlap)));
			double overlap_perc = 100.0 * overlap / (double)std::min(c_s1, c_s2);
			auto item = new QTableWidgetItem();
			item->setData(Qt::DisplayRole, overlap_perc); //set as display role instead of string to allow sorting
			table->setItem(row, 5, item);

			++row;
		}
	}

	table->sortByColumn(5, Qt::DescendingOrder);

	QApplication::restoreOverrideCursor();

	//show results
	auto dlg = GUIHelper::createDialog(table, "Sample similarity based on rare variants from NGSD", "Note: this method uses variants from NGSD only, i.e. variants with a maximum AF of 5%, to quickly find related samples.<br>"
																									"Using rare variants only, enriches for artefacts. Thus, the overlap numbers are not as accurate as with the 'sample similarity tool'!");
	dlg->exec();
}

QList<int> VariantWidget::selectedRows() const
{
	QSet<int> set;

	foreach(QTableWidgetItem* item, ui_.table->selectedItems())
	{
		set << item->row();
	}

    return set.values();
}

void VariantWidget::openProcessedSampleTabs()
{
	QList<int> rows = selectedRows();
	foreach(int row, rows)
	{
		QString ps = ui_.table->item(row, 0)->text();
		GlobalServiceProvider::openProcessedSampleTab(ps);
	}
}

void VariantWidget::openGeneTab(QString symbol)
{
	GlobalServiceProvider::openGeneTab(symbol);
}

void VariantWidget::openGSvarFile()
{
	QList<int> rows = selectedRows();

	//check that ony
	if (rows.count()!=1)
	{
		QMessageBox::warning(this,  "Open variant list", "Please select exactly one sampe to open!");
		return;
	}

	QString ps = ui_.table->item(rows[0], 0)->text();
	GlobalServiceProvider::openGSvarViaNGSD(ps, true);
}

void VariantWidget::editComment()
{
	try
	{
		//add variant if missing
		NGSD db;
		bool ok = true;
		QByteArray text = QInputDialog::getMultiLineText(this, "Variant comment", "Text: ", db.comment(variant_), &ok).toUtf8();
		if (!ok) return;

		db.setComment(variant_, text);
		updateGUI();
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
	}
}

void VariantWidget::editClassification()
{
	try
	{
		//execute dialog
		ClassificationDialog dlg(this, variant_);
		if (dlg.exec()!=QDialog::Accepted) return;

		//update NGSD
		NGSD db;
		db.setClassification(variant_, VariantList(), dlg.classificationInfo());

		updateGUI();
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
	}
}

void VariantWidget::gnomadClicked(QString var_id)
{
	NGSD db;
	Variant v = db.variant(var_id);
	QString link = GSvarHelper::gnomADLink(v);
	QDesktopServices::openUrl(QUrl(link));
}

void VariantWidget::pubmedClicked(QString link)
{
	if (Helper::isHttpUrl(link)) //transcript
	{
		QDesktopServices::openUrl(QUrl(link));
	}
	else //gene
	{
		//open all publications
		QStringList pubmed_ids = ui_.pubmed->toolTip().split(", ");
		foreach (QString id, pubmed_ids)
		{
			QDesktopServices::openUrl(QUrl("https://pubmed.ncbi.nlm.nih.gov/" + id + "/"));
		}
	}
}

void VariantWidget::tableCellDoubleClicked(int row, int /*column*/)
{
	QString ps = ui_.table->item(row, 0)->text();
	GlobalServiceProvider::openProcessedSampleTab(ps);
}

void VariantWidget::copyVariant()
{
	QString format = qobject_cast<QAction*>(sender())->text().trimmed();
	if (format=="GSvar")
	{
		QApplication::clipboard()->setText(variant_.toString());
	}
	else if (format=="VCF")
	{
		FastaFileIndex genome_idx(Settings::string("reference_genome"));
		VcfLine vcf = variant_.toVCF(genome_idx);
		QApplication::clipboard()->setText(vcf.chr().strNormalized(true) + "\t" + QString::number(vcf.start()) + "\t.\t" + vcf.ref() + "\t" + vcf.altString());
	}
	else if (format=="HGVS.c")
	{
		FastaFileIndex genome_idx(Settings::string("reference_genome"));
		QStringList output;
		NGSD db;
		GeneSet genes = db.genesOverlapping(variant_.chr(), variant_.start(), variant_.end());
		VariantHgvsAnnotator hgvs_annotator(genome_idx);
        for (const QByteArray& gene : genes)
		{
			Transcript trans = db.bestTranscript(db.geneId(gene));
			VariantConsequence consequence = hgvs_annotator.annotate(trans, variant_);
			output << trans.nameWithVersion() + ":" + consequence.hgvs_c;
		}
		QApplication::clipboard()->setText(output.join("\n"));
	}
	else if (format=="gnomAD")
	{
		FastaFileIndex genome_idx(Settings::string("reference_genome"));
		QApplication::clipboard()->setText(variant_.toGnomAD(genome_idx));
	}
	else
	{
		THROW(ProgrammingException, "Unprocessed format '" + format + "' selected!");
	}
}

void VariantWidget::openVariantInTab()
{
	VariantList variants;
	variants.append(variant_);

	VariantAnnotator* worker = new VariantAnnotator(variants);
	GlobalServiceProvider::startJob(worker, true);
}

