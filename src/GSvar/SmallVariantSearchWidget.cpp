#include "SmallVariantSearchWidget.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "NGSHelper.h"
#include "GlobalServiceProvider.h"
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>
#include <QMenu>

SmallVariantSearchWidget::SmallVariantSearchWidget(QWidget *parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, false)
{
	ui_.setupUi(this);
	setWindowFlags(Qt::Window);
	connect(&init_timer_, SIGNAL(triggerInitialization()), this, SLOT(updateVariants()));
	connect(ui_.radio_region->group(), SIGNAL(buttonToggled(int,bool)), this, SLOT(changeSearchType()));
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateVariants()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_.variants, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(variantContextMenu(QPoint)));
}

void SmallVariantSearchWidget::setGene(const QString& gene)
{
	ui_.genes->setText(gene);

	ui_.radio_genes->setChecked(true);
}

void SmallVariantSearchWidget::changeSearchType()
{
	ui_.region->setEnabled(ui_.radio_region->isChecked());
	ui_.genes->setEnabled(ui_.radio_genes->isChecked());
}

void SmallVariantSearchWidget::updateVariants()
{
	//clear old results
	ui_.variants->clearContents();

	//check if applicable
	if (ui_.radio_genes->isChecked() && ui_.genes->text().isEmpty()) return;
	if (ui_.radio_region->isChecked() && ui_.region->text().isEmpty()) return;

	//perform search
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//process genes/region
		QStringList comments;
		QList<QStringList> output;
		if (ui_.radio_genes->isChecked())
		{
			QByteArray text = ui_.genes->text().toLatin1().trimmed().replace(' ', ',');
			GeneSet genes = GeneSet::createFromText(text, ',');
			foreach(QByteArray gene, genes)
			{
				//check gene name
				NGSD db;
				int gene_id = db.geneToApprovedID(gene);
				if (gene_id==-1)
				{
					comments.append("Error: Gene name '" + gene + "' is not a HGNC-approved symbol => Skipping it!");
					continue;
				}
				gene = db.geneSymbol(gene_id);

				//get chromosomal range
				SqlQuery query = db.getQuery();
				query.exec("SELECT DISTINCT gt.chromosome, min(ge.start), max(ge.end) FROM gene_exon ge, gene_transcript gt WHERE gt.gene_id='" + QString::number(gene_id) + "' AND ge.transcript_id=gt.id GROUP BY gt.chromosome");
				if (query.size()!=1)
				{
					comments.append("Error: Could not determine chromosomal coordinates for gene '" + gene + "' => Skipping it!");
					continue;
				}
				query.next();
				Chromosome chr = query.value(0).toString();
				int start = query.value(1).toInt()-5000;
				int end = query.value(2).toInt()+5000;

				//get alternate gene symbols
				GeneSet gene_symbols;
				gene_symbols.insert(gene);
				gene_symbols.insert(db.synonymousSymbols(gene_id));
				gene_symbols.insert(db.previousSymbols(gene_id));

				getVariantsForRegion(chr, start, end, gene, gene_symbols, output, comments);
			}
		}
		else
		{
			Chromosome chr;
			int start, end;
			NGSHelper::parseRegion(ui_.region->text(), chr, start, end);

			getVariantsForRegion(chr, start, end, "<region>", GeneSet(), output, comments);
		}

		//show output
		ui_.variants->setSortingEnabled(false);
		ui_.variants->setRowCount(output.count());
		for (int r=0; r<output.count(); ++r)
		{
			const QStringList& line = output[r];
			for (int c=0; c<line.count(); ++c)
			{
				ui_.variants->setItem(r, c, new QTableWidgetItem(line[c]));
			}
		}
		ui_.variants->setSortingEnabled(true);

		//resize cols
		GUIHelper::resizeTableCells(ui_.variants, 600);

		//reset cursor
		QApplication::restoreOverrideCursor();

		//show comments
		QMessageBox::information(this, "Gene/variant statistics", comments.join("\n"));
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::critical(this, "Error", e.message());
	}
}

void SmallVariantSearchWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.variants);
}

void SmallVariantSearchWidget::variantContextMenu(QPoint pos)
{
	//get variant index
	QTableWidgetItem* item = ui_.variants->itemAt(pos);
	if (!item) return;

	QMenu menu(ui_.variants);
	QAction* action_var_tab = menu.addAction(QIcon(":/Icons/NGSD_variant.png"), "Open variant tab");
	menu.addSeparator();
	QAction* action_var_gsvar = menu.addAction("Copy variant (GSvar format)");

	//execute context menu
	QAction* action = menu.exec(ui_.variants->viewport()->mapToGlobal(pos));
	if (!action_var_gsvar) return;

	if (action==action_var_tab)
	{
		QString variant_string = ui_.variants->item(item->row(), 1)->text();
		GlobalServiceProvider::openVariantTab(Variant::fromString(variant_string));
	}
	if (action==action_var_gsvar)
	{
		QString variant_string = ui_.variants->item(item->row(), 1)->text();
		Variant v = Variant::fromString(variant_string);
		QApplication::clipboard()->setText(v.toString());
	}
}

void SmallVariantSearchWidget::getVariantsForRegion(Chromosome chr, int start, int end, QByteArray gene, const GeneSet& gene_symbols, QList<QStringList>& output, QStringList& messages)
{
	NGSD db;

	//impacts
	QStringList impacts;
	if (ui_.filter_impact_high->isChecked()) impacts << "HIGH";
	if (ui_.filter_impact_moderate->isChecked()) impacts << "MODERATE";
	if (ui_.filter_impact_low->isChecked()) impacts << "LOW";
	if (ui_.filter_impact_modifier->isChecked()) impacts << "MODIFIER";

	//processed sample quality
	QStringList ps_qualities;
	if (ui_.q_ps_good->isChecked()) ps_qualities << "good";
	if (ui_.q_ps_medium->isChecked()) ps_qualities << "medium";
	if (ui_.q_ps_bad->isChecked()) ps_qualities << "bad";
	if (ui_.q_ps_na->isChecked()) ps_qualities << "n/a";

	//project types
	QStringList project_types;
	if (ui_.p_diagnostic->isChecked()) project_types << "diagnostic";
	if (ui_.p_research->isChecked()) project_types << "research";
	if (ui_.p_external->isChecked()) project_types << "external";
	if (ui_.p_test->isChecked()) project_types << "test";

	//processing system types
	QStringList sys_types;
	if (ui_.filter_sys_wgs->isChecked()) sys_types << "WGS";
	if (ui_.filter_sys_wes->isChecked()) sys_types << "WES";
	if (ui_.filter_sys_other->isChecked())
	{
		QStringList types_other = db.getEnum("processing_system", "type");
		types_other.removeAll("WGS");
		types_other.removeAll("WES");
		sys_types << types_other;
	}

	//get variants in chromosomal range
	QSet<QString> vars_distinct;
	QList<QStringList> var_data;
	QString max_af = QString::number(ui_.filter_af->value()/100.0);
	int max_ngsd = ui_.filter_ngsd_count->value();
	QString query_text = "SELECT v.* FROM variant v WHERE chr='" + chr.strNormalized(true) + "' AND start>='" + QString::number(start) + "' AND end<='" + QString::number(end) + "' AND (1000g IS NULL OR 1000g<=" + max_af + ") AND (gnomad IS NULL OR gnomad<=" + max_af + ") ORDER BY start";
	SqlQuery query = db.getQuery();
	query.exec(query_text);
	while(query.next())
	{
		QString var = query.value("chr").toString() + ":" + query.value("start").toString() + "-" + query.value("end").toString() + " " + query.value("ref").toString() + " > " + query.value("obs").toString();
		QString gnomad = QString::number(query.value("gnomad").toDouble(), 'f', 4);
		QString tg = QString::number(query.value("1000g").toDouble(), 'f', 4);

		//filter by impact
		QStringList parts = query.value("coding").toString().split(",");
		QStringList parts_match;
		foreach(const QString& part, parts)
		{
			int index = part.indexOf(':');
			if (index==-1) continue;
			QByteArray current_gene = part.left(index).toLatin1();
			if (!gene_symbols.isEmpty() && !gene_symbols.contains(current_gene)) continue;

			bool match = false;
			foreach(const QString& impact, impacts)
			{
				if (part.contains(impact))
				{
					match = true;
				}
			}
			if (match)
			{
				parts_match << part;
			}
		}

		if (parts_match.count()==0) continue;

		//determine NGSD hom/het counts
		QString variant_id = query.value("id").toString();
		QPair<int, int> ngsd_counts = db.variantCounts(variant_id);
		if (ngsd_counts.first + ngsd_counts.second ==0) continue; //skip somatic-only variants

		//apply NGSD count filter
		if (max_ngsd>0 && (ngsd_counts.first + ngsd_counts.second)>max_ngsd) continue;

		//format transcript info
		QSet<QString> types;
		foreach(const QString& part, parts_match)
		{
			QStringList parts2 = part.split(":");
			types.insert(parts2[2]);
		}
		QString type = types.toList().join(", ");
		QString coding = parts_match.join(", ");

		//add sample info
		SqlQuery query2 = db.getQuery();
		query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps_name, dv.genotype, p.name as p_name, s.disease_group, s.disease_status, vc.class, s.name_external, ds.outcome, ds.comment, s.id as s_id, ps.id as ps_id, sys.type as sys_type, sys.name_manufacturer as sys_name, p.type as p_type, ps.quality as ps_quality FROM sample s, processed_sample ps LEFT JOIN diag_status ds ON ps.id=ds.processed_sample_id, project p, detected_variant dv LEFT JOIN variant_classification vc ON dv.variant_id=vc.variant_id, processing_system sys WHERE ps.processing_system_id=sys.id AND dv.processed_sample_id=ps.id AND ps.sample_id=s.id AND ps.project_id=p.id AND dv.variant_id=" + variant_id);
		while(query2.next())
		{

			//filter by processed sample quality
			QString ps_quality = query2.value("ps_quality").toString();
			if (!ps_qualities.contains(ps_quality)) continue;

			//filter by processing system type
			QString sys_type = query2.value("sys_type").toString();
			if (!sys_types.contains(sys_type)) continue;

			//filter by project type
			QString p_type = query2.value("p_type").toString();
			if (!project_types.contains(p_type)) continue;

			//get HPO info
			QString sample_id = query2.value("s_id").toString();
			QString processed_sample_id = query2.value("ps_id").toString();
			PhenotypeList phenotypes = db.samplePhenotypes(sample_id);

			//get causal genes from report config
			GeneSet genes_causal;
			SqlQuery query3 = db.getQuery();
			query3.exec("SELECT v.chr, v.start, v.end FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='diagnostic variant' AND rcv.causal=1 AND rc.processed_sample_id=" + processed_sample_id);
			while(query3.next())
			{
				genes_causal << db.genesOverlapping(query3.value(0).toByteArray(), query3.value(1).toInt(), query3.value(2).toInt(), 5000);
			}

			//get candidate genes from report config
			GeneSet genes_candidate;
			query3.exec("SELECT v.chr, v.start, v.end FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='candidate variant' AND rc.processed_sample_id=" + processed_sample_id);
			while(query3.next())
			{
				genes_candidate << db.genesOverlapping(query3.value(0).toByteArray(), query3.value(1).toInt(), query3.value(2).toInt(), 5000);
			}

			//get de-novo variants from report config
			query3.exec("SELECT rcv.id FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.de_novo=1 AND rc.processed_sample_id=" + processed_sample_id + " AND v.id=" + variant_id);
			QString denovo = query3.size()==0 ? "" : " (de-novo)";

			//get related sample info
			QStringList related_samples;
			query3.exec("SELECT sr.sample1_id, s1.name as s1_name, s2.name as s2_name, sr.relation FROM sample s1, sample s2, sample_relations sr WHERE s1.id=sr.sample1_id AND s2.id=sample2_id AND (sr.sample1_id="+sample_id+" OR sr.sample2_id="+sample_id+")");
			while(query3.next())
			{
				QString relation = query3.value("relation").toString();
				if (relation=="parent-child" || relation=="sibling" || relation=="same sample" || relation=="same patient")
				{
					related_samples << relation + ":" + (query3.value("sample1_id").toString()==sample_id ? query3.value("s2_name").toString() : query3.value("s1_name").toString());
				}
			}

			//add variant line to output
			vars_distinct << variant_id;
			var_data.append(QStringList() << gene << var << QString::number(ngsd_counts.first) << QString::number(ngsd_counts.second) << gnomad << tg << type << coding << query2.value("ps_name").toString() << query2.value("name_external").toString()  << query2.value("genotype").toString() + denovo << query2.value("sys_name").toString()<< query2.value("p_name").toString() << query2.value("disease_group").toString() << query2.value("disease_status").toString() << phenotypes.toString() << query2.value("class").toString() << query2.value("outcome").toString() << query2.value("comment").toString().replace("\n", " ") << genes_causal.join(',') << genes_candidate.join(',')<< related_samples.join(", "));
		}
	}
	QString comment = gene + " - " + QString::number(vars_distinct.count()) + " distinct variants in " + QString::number(var_data.count()) + " hits";

	//only variants that fit recessive inheritance mode
	if (ui_.filter_recessive->isChecked())
	{
		int i_ps = 8;
		int i_geno = 10;

		//count heterozygous hits per sample
		QMap<QString, int> het_hits;
		foreach(const QStringList& line, var_data)
		{
			if (line[i_geno]=="het")
			{
				het_hits[line[i_ps]] += 1;
			}
		}
		//qDebug() << het_hits;

		//remove samples with less than two hits
		var_data.erase(std::remove_if(var_data.begin(), var_data.end(), [het_hits, i_ps, i_geno](const QStringList& line){return !(line[i_geno]=="hom" || het_hits[line[i_ps]]>=2);}), var_data.end());

		comment += " - recessive hits: " + QString::number(var_data.count());
	}

	//only variants that fit de-novo inheritance mode
	if (ui_.filter_denovo->isChecked())
	{
		int i_geno = 10;

		var_data.erase(std::remove_if(var_data.begin(), var_data.end(), [i_geno](const QStringList& line){return !line[i_geno].contains("(de-novo)");}), var_data.end());

		comment += " - de-novo hits: " + QString::number(var_data.count());
	}

	//output
	messages << comment;
	output << var_data;
}
