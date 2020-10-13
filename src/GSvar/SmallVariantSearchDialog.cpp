#include "SmallVariantSearchDialog.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "NGSHelper.h"
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>

SmallVariantSearchDialog::SmallVariantSearchDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
	, init_timer_(this, false)
{
	ui_.setupUi(this);
	connect(&init_timer_, SIGNAL(triggerInitialization()), this, SLOT(updateVariants()));

	connect(ui_.radio_region->group(), SIGNAL(buttonToggled(int,bool)), this, SLOT(changeSearchType()));

	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateVariants()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
}

void SmallVariantSearchDialog::setGene(const QString& gene)
{
	ui_.genes->setText(gene);

	ui_.radio_genes->setChecked(true);
}

void SmallVariantSearchDialog::changeSearchType()
{
	ui_.region->setEnabled(ui_.radio_region->isChecked());
	ui_.genes->setEnabled(ui_.radio_genes->isChecked());
}

void SmallVariantSearchDialog::updateVariants()
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

void SmallVariantSearchDialog::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.variants);
}

void SmallVariantSearchDialog::getVariantsForRegion(Chromosome chr, int start, int end, QByteArray gene, const GeneSet& gene_symbols, QList<QStringList>& output, QStringList& messages)
{
	NGSD db;

	//impacts
	QStringList impacts;
	if (ui_.filter_impact_high->isChecked()) impacts << "HIGH";
	if (ui_.filter_impact_moderate->isChecked()) impacts << "MODERATE";
	if (ui_.filter_impact_low->isChecked()) impacts << "LOW";
	if (ui_.filter_impact_modifier->isChecked()) impacts << "MODIFIER";

	//get variants in chromosomal range
	QList<QStringList> var_data;
	QString af = QString::number(ui_.filter_af->value()/100.0);
	QString query_text = "SELECT v.* FROM variant v WHERE chr='" + chr.strNormalized(true) + "' AND start>='" + QString::number(start) + "' AND end<='" + QString::number(end) + "' AND (1000g IS NULL OR 1000g<=" + af + ") AND (gnomad IS NULL OR gnomad<=" + af + ") ORDER BY start";
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
		query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps_name, dv.genotype, p.name as p_name, s.disease_group, vc.class, s.name_external, ds.outcome, ds.comment, s.id as s_id, ps.id as ps_id FROM sample s, processed_sample ps LEFT JOIN diag_status ds ON ps.id=ds.processed_sample_id, project p, detected_variant dv LEFT JOIN variant_classification vc ON dv.variant_id=vc.variant_id WHERE dv.processed_sample_id=ps.id AND ps.sample_id=s.id AND ps.project_id=p.id AND dv.variant_id=" + variant_id);
		while(query2.next())
		{
			//get HPO info
			QStringList hpo_terms;
			QString sample_id = query2.value("s_id").toString();
			QString processed_sample_id = query2.value("ps_id").toString();
			QStringList hpo_ids = db.getValues("SELECT disease_info FROM sample_disease_info WHERE type='HPO term id' AND sample_id=" + sample_id);
			foreach(QString hpo_id, hpo_ids)
			{
				Phenotype pheno = db.phenotypeByAccession(hpo_id.toLatin1(), false);
				if (!pheno.name().isEmpty())
				{
					hpo_terms << pheno.toString();
				}
			}

			//get causal genes from report config
			GeneSet genes_causal;
			SqlQuery query3 = db.getQuery();
			query3.exec("SELECT v.gene FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='diagnostic variant' AND rcv.causal=1 AND rc.processed_sample_id=" + processed_sample_id);
			while(query3.next())
			{
				genes_causal << query3.value(0).toByteArray().split(',');
			}

			//get candidate genes from report config
			GeneSet genes_candidate;
			query3.exec("SELECT v.gene FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='candidate variant' AND rc.processed_sample_id=" + processed_sample_id);
			while(query3.next())
			{
				genes_candidate << query3.value(0).toByteArray().split(',');
			}

			//get de-novo variants from report config
			query3.exec("SELECT rcv.id FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.de_novo=1 AND rc.processed_sample_id=" + processed_sample_id + " AND v.id=" + variant_id);
			QString denovo = query3.size()==0 ? "" : " (de-novo)";

			//add variant line to output
			var_data.append(QStringList() << gene << var << QString::number(ngsd_counts.first) << QString::number(ngsd_counts.second) << gnomad << tg << type << coding << query2.value("ps_name").toString() << query2.value("name_external").toString()  << query2.value("genotype").toString() + denovo << query2.value("p_name").toString() << query2.value("disease_group").toString() << hpo_terms.join("; ") << query2.value("class").toString() << query2.value("outcome").toString() << query2.value("comment").toString().replace("\n", " ") << genes_causal.join(',') << genes_candidate.join(','));
		}
	}
	QString comment = gene + " - variants: " + QString::number(var_data.count());

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
