#include "CandidateGeneDialog.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QKeyEvent>

CandidateGeneDialog::CandidateGeneDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateVariants()));
	connect(ui_.copy_btn, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
}

void CandidateGeneDialog::updateVariants()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//clear old results
	ui_.variants->clearContents();

	//init
	NGSD db;
	QStringList impacts;
	if (ui_.filter_impact_high->isChecked()) impacts << "HIGH";
	if (ui_.filter_impact_moderate->isChecked()) impacts << "MODERATE";
	if (ui_.filter_impact_low->isChecked()) impacts << "LOW";
	if (ui_.filter_impact_modifier->isChecked()) impacts << "MODIFIER";
	bool only_basic_transcripts = ui_.filter_gencode_basic->isChecked();

	//process
	QStringList comments;
	QList<QStringList> output;
	QByteArrayList genes = ui_.genes->text().toLatin1().replace(' ', ',').split(',');
	foreach(QByteArray gene, genes)
	{
		if (gene.trimmed()=="") continue;

		//check gene name
		int gene_id = db.geneToApprovedID(gene);
		if (gene_id==-1)
		{
			comments.append("Error: Gene name '" + gene + "' is not a HGNC-approved symbol => Skipping it!");
			continue;
		}
		gene = db.geneSymbol(gene_id);

		//get GENCODE basic transcripts
		QStringList basic_transcripts;
		if (only_basic_transcripts)
		{
			basic_transcripts = db.getValues("SELECT name FROM gene_transcript WHERE gene_id='" + QString::number(gene_id) + "'");
		}

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
		QString start =  QString::number(query.value(1).toInt()-20);
		QString end =  QString::number(query.value(2).toInt()+20);

		//get alternate gene symbols
		GeneSet gene_symbols;
		gene_symbols.insert(gene);
		gene_symbols.insert(db.synonymousSymbols(gene_id));
		gene_symbols.insert(db.previousSymbols(gene_id));

		//get variants in chromosomal range
		QList<QStringList> var_data;
		QString af = QString::number(ui_.filter_af->value()/100.0);
		QString query_text = "SELECT v.*, dvc.count_het, dvc.count_hom FROM variant v LEFT JOIN detected_variant_counts dvc ON v.id=dvc.variant_id WHERE chr='" + chr.strNormalized(true) + "' AND start>='" + start + "' AND end<'" + end + "' AND (1000g IS NULL OR 1000g<=" + af + ") AND (gnomad IS NULL OR gnomad<=" + af + ") ORDER BY start";
		query.exec(query_text);
		while(query.next())
		{
			QString var = query.value("chr").toString() + ":" + query.value("start").toString() + "-" + query.value("end").toString() + " " + query.value("ref").toString() + " > " + query.value("obs").toString();
			QString het = query.value("count_het").toString();
			QString hom = query.value("count_hom").toString();
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
				if (!gene_symbols.contains(current_gene)) continue;

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

			//filter by GENCODE basic transcripts
			if (only_basic_transcripts)
			{
				parts = parts_match;
				parts_match.clear();

				foreach(const QString& part, parts)
				{
					foreach(const QString& transcript_id, basic_transcripts)
					{
						if (part.contains(transcript_id))
						{
							parts_match << part;
						}
					}
				}
			}

			if (parts_match.count()==0) continue;

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
			QString var_id = query.value(0).toString();
			SqlQuery query2 = db.getQuery();
			query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), dv.genotype, p.name, s.disease_group, vc.class, s.name_external, ds.outcome, ds.comment, s.id, ps.id FROM sample s, processed_sample ps LEFT JOIN diag_status ds ON ps.id=ds.processed_sample_id, project p, detected_variant dv LEFT JOIN variant_classification vc ON dv.variant_id=vc.variant_id WHERE dv.processed_sample_id=ps.id AND ps.sample_id=s.id AND ps.project_id=p.id AND dv.variant_id=" + var_id);
			while(query2.next())
			{
				//get HPO info
				QStringList hpo_terms;
				QString sample_id = query2.value(8).toString();
				QString processed_sample_id = query2.value(9).toString();
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
				SqlQuery query4 = db.getQuery();
				query4.exec("SELECT v.gene FROM variant v, report_configuration rc, report_configuration_variant rcv WHERE v.id=rcv.variant_id AND rcv.report_configuration_id=rc.id AND rcv.type='candidate variant' AND rc.processed_sample_id=" + processed_sample_id);
				while(query4.next())
				{
					genes_candidate << query4.value(0).toByteArray().split(',');
				}

				//add variant line to output
				var_data.append(QStringList() << gene << var << het << hom << gnomad << tg << type << coding << query2.value(0).toString() << query2.value(5).toString()  << query2.value(1).toString() << query2.value(2).toString() << query2.value(3).toString() << hpo_terms.join("; ") << query2.value(4).toString() << query2.value(6).toString() << query2.value(7).toString().replace("\n", " ") << genes_causal.join(',') << genes_candidate.join(','));
			}
		}
		QString comment = gene + " - variants: " + QString::number(var_data.count());

		//only samples that fit recessive inheritance mode
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

		//diagnostic status information
		comments.append(comment);
		output << var_data;
	}

	//show output
	ui_.variants->setRowCount(output.count());
	for (int r=0; r<output.count(); ++r)
	{
		const QStringList& line = output[r];
		for (int c=0; c<line.count(); ++c)
		{
			ui_.variants->setItem(r, c, new QTableWidgetItem(line[c]));
		}
	}

	//resize cols
	GUIHelper::resizeTableCells(ui_.variants, 600);

	//reset cursor
	QApplication::restoreOverrideCursor();

	//show comments
	QMessageBox::information(this, "Gene/variant statistics", comments.join("\n"));
}

void CandidateGeneDialog::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_.variants);
}
