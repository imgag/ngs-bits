#include "CandidateGeneDialog.h"
#include "NGSD.h"
#include <QDebug>
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
}

void CandidateGeneDialog::keyPressEvent(QKeyEvent* e)
{
	if(e->key() == Qt::Key_C && e->modifiers() & Qt::ControlModifier)
	{
		copyToClipboard();
		e->accept();
	}
}

void CandidateGeneDialog::updateVariants()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	//clear old results
	ui_.variants->clearContents();

	//init
	NGSD db;
	QStringList impacts = ui_.filter_impact->currentText().split(",");
	bool only_basic_transcripts = ui_.filter_gencode_basic->isChecked();

	//process
	QStringList comments;
	QList<QStringList> output;
	QStringList genes = ui_.genes->text().replace(" ", ",").split(",",QString::SkipEmptyParts);
	foreach(QString gene, genes)
	{
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

		//get variants in chromosomal range
		QList<QStringList> var_data;
		QString af = QString::number(ui_.filter_af->value()/100.0);
		query.exec("SELECT v.*, dvc.count_het, dvc.count_hom FROM variant v, detected_variant_counts dvc WHERE v.id=dvc.variant_id AND chr='" + chr.strNormalized(true) + "' AND start>='" + start + "' AND end<'" + end + "' AND (1000g IS NULL OR 1000g<=" + af + ") AND (exac IS NULL OR exac<=" + af + ") AND (gnomad IS NULL OR gnomad<=" + af + ") ORDER BY start");
		while(query.next())
		{
			QString var = query.value("chr").toString() + ":" + query.value("start").toString() + "-" + query.value("end").toString() + " " + query.value("ref").toString() + " > " + query.value("obs").toString();
			QString het = query.value("count_het").toString();
			QString hom = query.value("count_hom").toString();
			QString gnomad = QString::number(query.value("gnomad").toDouble(), 'f', 4);
			QString exac = QString::number(query.value("exac").toDouble(), 'f', 4);
			QString tg = QString::number(query.value("1000g").toDouble(), 'f', 4);

			//exclude somatic variants
			if (het.toInt()+hom.toInt()==0) continue;

			//filter by impact
			QStringList parts = query.value("coding").toString().split(",");
			QStringList parts_match;
			foreach(QString part, parts)
			{
				if (!part.startsWith(gene + ":")) continue;

				foreach(QString impact, impacts)
				{
					if (part.contains(impact))
					{
						parts_match << part;
					}
				}
			}

			//filter by GENCODE basic transcripts
			if (only_basic_transcripts)
			{
				parts = parts_match;
				parts_match.clear();

				foreach(QString part, parts)
				{
					foreach(QString transcript_id, basic_transcripts)
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
			foreach(QString part, parts_match)
			{
				QStringList parts2 = part.split(":");
				types.insert(parts2[2]);
			}
			QString type = types.toList().join(", ");
			QString coding = parts_match.join(", ");

			//add variant line to output
			QStringList var_base = QStringList() << gene << var << het << hom << gnomad << exac << tg << type << coding;

			//add sample info
			QString var_id = query.value(0).toString();
			SqlQuery query2 = db.getQuery();
			query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), dv.genotype, p.name, s.disease_group FROM detected_variant dv, sample s, processed_sample ps, project p WHERE dv.processed_sample_id=ps.id AND ps.sample_id=s.id AND ps.project_id=p.id AND dv.variant_id=" + var_id);
			while(query2.next())
			{
				var_data.append(QStringList() << var_base << query2.value(0).toString() << query2.value(1).toString() << query2.value(2).toString() << query2.value(3).toString());
			}
		}
		QString comment = gene + " - variants: " + QString::number(var_data.count());

		//only samples that fit recessive inheritance mode
		if (ui_.filter_recessive->isChecked())
		{
			//count hits per sample
			QMap<QString, int> hits;
			foreach(const QStringList& line, var_data)
			{
				hits[line[9]] += line[10]=="hom" ? 2 : 1;
			}

			//remove samples with less than two hits
			var_data.erase(std::remove_if(var_data.begin(), var_data.end(), [hits](const QStringList& line){return hits[line[9]]<2;}), var_data.end());

			comment += " - recessive hits: " + QString::number(var_data.count());
		}

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
	ui_.variants->resizeColumnsToContents();
	for (int i=0; i<ui_.variants->columnCount(); ++i)
	{
		if (ui_.variants->columnWidth(i)>600)
		{
			ui_.variants->setColumnWidth(i, 600);
		}
	}

	//reset cursor
	QApplication::restoreOverrideCursor();

	//show comments
	QMessageBox::information(this, "Gene/variant statistics", comments.join("\n"));
}

void CandidateGeneDialog::copyToClipboard()
{
	//header
	QString output = "#";
	for (int col=0; col<ui_.variants->columnCount(); ++col)
	{
		if (col!=0) output += "\t";
		output += ui_.variants->horizontalHeaderItem(col)->text();
	}
	output += "\n";

	//rows
	for (int row=0; row<ui_.variants->rowCount(); ++row)
	{
		if (ui_.variants->isRowHidden(row)) continue;

		for (int col=0; col<ui_.variants->columnCount(); ++col)
		{
			if (col!=0) output += "\t";
			output += ui_.variants->item(row, col)->text();
		}
		output += "\n";
	}

	QApplication::clipboard()->setText(output);
}
