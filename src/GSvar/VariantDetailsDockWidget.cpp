#include "VariantDetailsDockWidget.h"
#include "ui_VariantDetailsDockWidget.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QDebug>
#include <QPixmap>
#include <QMessageBox>

VariantDetailsDockWidget::VariantDetailsDockWidget(QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::VariantDetailsDockWidget)
{
	ui->setupUi(this);

	//signals + slots
	connect(ui->trans_prev, SIGNAL(clicked(bool)), this, SLOT(previousTanscript()));
	connect(ui->trans_next, SIGNAL(clicked(bool)), this, SLOT(nextTanscript()));

	//set up transcript buttons
	ui->trans_prev->setStyleSheet("QPushButton {border: none; margin: 0px;padding: 0px;}");
	ui->trans_next->setStyleSheet("QPushButton {border: none; margin: 0px;padding: 0px;}");

	//set up content label properties
	QList<QLabel*> labels = findChildren<QLabel*>();
	foreach(QLabel* label, labels)
	{
		if (label->objectName().startsWith("label")) continue;
		int width = label->minimumWidth();
		if (width==0) width = 200;
		label->setMinimumWidth(width);
		label->setWordWrap(true);
		label->setOpenExternalLinks(true);
	}

	//reset
	clear();
}

VariantDetailsDockWidget::~VariantDetailsDockWidget()
{
	delete ui;
}

void VariantDetailsDockWidget::setPreferredTranscripts(QMap<QString, QString> data)
{
	preferred_transcripts = data;
}

void VariantDetailsDockWidget::updateVariant(const VariantList& vl, int index)
{
	//variant
	ui->variant->setText(vl[index].toString());

	//details
	initTranscriptDetails(vl, index);

	//base information
	setAnnotation(ui->genotype, vl, index, "genotype");
	setAnnotation(ui->gene, vl, index, "gene");
	setAnnotation(ui->quality, vl, index, "quality");

	//public databases
	setAnnotation(ui->dbsnp, vl, index, "dbSNP");
	setAnnotation(ui->clinvar, vl, index, "ClinVar");
	setAnnotation(ui->hgmd, vl, index, "HGMD");
	setAnnotation(ui->omim, vl, index, "OMIM");
	setAnnotation(ui->cosmic, vl, index, "COSMIC");

	//public allel frequencies
	setAnnotation(ui->tg, vl, index, "1000g");
	setAnnotation(ui->exac, vl, index, "ExAC");
	setAnnotation(ui->esp6500aa, vl, index, "ESP6500AA");
	setAnnotation(ui->esp6500ea, vl, index, "ESP6500EA");

	//pathogenity predictions
	setAnnotation(ui->phylop, vl, index, "phyloP");
	setAnnotation(ui->metalr, vl, index, "MetaLR");
	setAnnotation(ui->sift, vl, index, "Sift");
	setAnnotation(ui->pp2_hvar, vl, index, "PP2_HVAR");
	setAnnotation(ui->pp2_hdiv, vl, index, "PP2_HDIV");

	//NGSD
	setAnnotation(ui->ngsd_class, vl, index, "classification");
	setAnnotation(ui->ngsd_hom, vl, index, "ihdb_allsys_hom");
	setAnnotation(ui->ngsd_het, vl, index, "ihdb_allsys_het");
	setAnnotation(ui->ngsd_comment, vl, index, "comment");
	setAnnotation(ui->ngsd_validation, vl, index, "validated");
}

void VariantDetailsDockWidget::clear()
{
	//clear content labels
	QList<QLabel*> labels = findChildren<QLabel*>();
	foreach(QLabel* label, labels)
	{
		if (label->objectName().startsWith("label")) continue;
		label->clear();
		label->setToolTip("");
	}

	//variant
	ui->variant->setText("No variant or several variants selected");

	//details
	ui->trans->setText("<span style=\"font-weight:600; color:#222222;\">&nbsp;<span>"); //bold => higher
	ui->trans_prev->setEnabled(false);
	ui->trans_next->setEnabled(false);
}

void VariantDetailsDockWidget::setAnnotation(QLabel* label, const VariantList& vl, int index, QString name)
{
	//init
	QString tooltip;
	QString text;

	//get annotation column index
	int a_index = vl.annotationIndexByName(name, true, false);
	if(a_index!=-1)
	{
		QString anno = vl[index].annotations()[a_index];

		//special handling of fields
		if (name=="dbSNP")
		{
			text = formatLink(anno, "http://www.ncbi.nlm.nih.gov/projects/SNP/snp_ref.cgi?rs=" + anno.mid(2));
		}
		else if(name=="OMIM")
		{
			foreach(const DBEntry& entry, parseDB(anno))
			{
				text += formatLink(entry.id, "http://omim.org/entry/" + entry.id) + " ";
				tooltip += nobr() + entry.id + ": " + entry.details;
			}
		}
		else if(name=="ClinVar")
		{
			foreach(const DBEntry& entry, parseDB(anno))
			{
				//determine color
				Color color = NONE;
				if (entry.details.contains("likely pathogenic")) color = ORANGE;
				else if (entry.details.contains("pathogenic")) color = RED;
				else if (entry.details.contains("benign")) color = GREEN;

				text += formatLink(entry.id, "http://www.ncbi.nlm.nih.gov/clinvar/" + entry.id, color) + " ";
				tooltip += nobr() + entry.id + ": " + entry.details;
			}
		}
		else if(name=="HGMD")
		{
			foreach(const DBEntry& entry, parseDB(anno))
			{
				//determine gene (needed for URL)
				QStringList gene_parts = entry.details.split("GENE=");
				if (gene_parts.count()<2) THROW(ProgrammingException, "No gene information found in HGMD entry: " + entry.id + " / " + entry.details);
				QString gene = gene_parts[1].left(gene_parts[1].length());

				//determine color
				Color color = NONE;
				if (entry.details.contains("CLASS=DM?")) color = ORANGE;
				else if (entry.details.contains("CLASS=DM")) color = RED;
				else if (entry.details.contains("CLASS=DP") || entry.details.contains("CLASS=DFP")) color = ORANGE;

				text += formatLink(entry.id, "http://www.hgmd.cf.ac.uk/ac/gene.php?gene=" + gene + "&accession=" + entry.id, color) + " ";
				tooltip += nobr() + entry.id + ": " + entry.details;
			}
		}
		else if(name=="COSMIC")
		{
			QStringList ids = anno.split(", ");
			foreach(QString id, ids)
			{
				id = id.trimmed();
				if (id.isEmpty()) continue;

				if (id.startsWith("COSM"))
				{
					text += formatLink(id.mid(4), "http://cancer.sanger.ac.uk/cosmic/mutation/overview?id=" + id) + " ";
				}
				else
				{
					text += formatLink(id.mid(4), "http://cancer.sanger.ac.uk/cosmic/ncv/overview?id=" + id) + " ";
				}
			}
		}
		else if(name=="quality")
		{
			text = anno.replace(';', ' ');
		}
		else if(name=="filter")
		{
			text = anno.replace(';', ' ');
		}
		else if(name=="phyloP")
		{
			bool ok = true;
			double value = anno.toDouble(&ok);
			if (ok && value>=1.6)
			{
				text = formatText(anno, RED);
			}
			else
			{
				text = anno;
			}
		}
		else if(name=="MetaLR" || name=="Sift" || name=="PP2_HVAR" || name=="PP2_HDIV")
		{
			text = anno.replace("D", formatText("D", RED)).replace("P", formatText("P", ORANGE));
		}
		else if(name=="ihdb_allsys_hom" || name=="ihdb_allsys_het")
		{
			bool ok = true;
			int value = anno.toInt(&ok);
			if (ok && value>50)
			{
				text = formatText(anno, GREEN);
			}
			else
			{
				text = anno;
			}
		}
		else if(name=="classification")
		{
			bool ok = true;
			int value = anno.toInt(&ok);
			if (ok && value==1)
			{
				text = formatText("benign (1)", GREEN);
			}
			else if (ok && value==2)
			{
				text = formatText("likely benign (2)", GREEN);
			}
			else if (ok && value==3)
			{
				text = formatText("unclear significance (3)", ORANGE);
			}
			else if (ok && value==4)
			{
				text = formatText("likely pathogenic (4)", ORANGE);
			}
			else if (ok && value==5)
			{
				text = formatText("pathogenic (5)", RED);
			}
			else if (anno=="M")
			{
				text = formatText("modifier (M)", ORANGE);
			}
			else if (anno!="" && anno!="n/a")
			{
				QMessageBox::critical(this, "Variant details error", "Unknown variant classification '" + anno + "'.\nPlease update the NGSD annotations!");
			}

			//classification comment
			int c_index = vl.annotationIndexByName("classification_comment", true, false);
			if(c_index!=-1)
			{
				tooltip = vl[index].annotations()[c_index];
			}
		}
		else if(name=="1000g" || name=="ExAC" || name=="ESP6500EA" || name=="ESP6500AA")
		{
			bool ok = true;
			double value = anno.toDouble(&ok);
			if (ok && value>=0.05)
			{
				text = formatText(anno, GREEN);
			}
			else
			{
				text = anno;
			}
		}
		else //fallback: use complete annotations string
		{
			text = anno;
		}
	}

	//set text/tooltip
	label->setText(text);
	label->setToolTip(tooltip);
}

QString VariantDetailsDockWidget::colorToString(VariantDetailsDockWidget::Color color)
{
	switch(color)
	{
		case NONE:
			return "";
		case GREEN:
			return "rgba(0, 255, 0, 128)";
		case ORANGE:
			return "rgba(255, 100, 0, 128)";
		case RED:
			return "rgba(255, 0, 0, 128)";
	};

	THROW(ProgrammingException, "Unkonwn color!");
}


void VariantDetailsDockWidget::initTranscriptDetails(const VariantList& vl, int index)
{
	//clear
	ui->trans->setText("<span style=\"font-weight:600; color:#222222;\">&nbsp;<span>"); //bold => higher
	ui->trans->setToolTip(QString());
	ui->trans_prev->setEnabled(false);
	ui->trans_next->setEnabled(false);
	trans_curr = -1;
	trans_data.clear();

	int a_index = vl.annotationIndexByName("coding_and_splicing", true, false);
	if(a_index==-1) return;

	//parse transcript data
	int high_impact = -1;
	QList<QByteArray> transcripts = vl[index].annotations()[a_index].split(',');
	foreach(QByteArray t, transcripts)
	{
		QStringList parts = QString(t).split(':');

		//fix protein-protein-contact entries
		if (t.contains("protein_protein_contact"))
		{
			parts = QStringList() << parts[0] << parts[1] << parts[4] << parts[5] << parts[6] << parts[7] << parts[8];
		}

		//check number of parts
		if (parts.count()!=7)
		{
			THROW(ProgrammingException, "coding_and_splicing annotation contains " + QString::number(parts.count()) + " parts (7 expected)!");
		}

		trans_data.append(parts);

		//store first high-impact variant
		if (high_impact==-1 && parts[3].contains("HIGH"))
		{
			high_impact = trans_data.count()-1;
		}
	}

	//set first transcript (or first high-impact if any)
	if (trans_data.count()>0)
	{
		setTranscript(high_impact==-1 ? 0 : high_impact);
	}

	//tooltip if more than one transcipt
	if (trans_data.count()>1)
	{
		QString tooltip;
		for(int i=0; i<trans_data.count(); ++i)
		{
			//highlight preferred transcripts
			bool is_pt = false;
			QString gene = trans_data[i][0];
			if (preferred_transcripts.contains(gene))
			{
				QString transcript = trans_data[i][1];
				if (transcript.startsWith(preferred_transcripts[gene])) is_pt = true;
			}

			tooltip += nobr() + (is_pt ? "<b>" : "") + transcripts[i] + (is_pt ? "</b>" : "");
		}
		ui->trans->setToolTip(tooltip);
	}
}

void VariantDetailsDockWidget::setTranscript(int index)
{
	trans_curr = index;

	//set transcript label
	QString text = trans_data[trans_curr][0] + " " + trans_data[trans_curr][1];
	if (trans_data.count()>1)
	{
		text += " (" + QString::number(trans_curr+1) + "/" + QString::number(trans_data.count()) + ")";
	}
	ui->trans->setText("<span style=\"font-weight:600; color:#222222;\">" + text + "<span>");

	//set detail labels
	QStringList parts = trans_data[index];
	bool high_impact = parts[3].contains("HIGH");
	if (high_impact)
	{
		ui->detail_type->setText(formatText(parts[2], RED));
		ui->detail_impact->setText(formatText(parts[3], RED));
	}
	else
	{
		ui->detail_type->setText(parts[2]);
		ui->detail_impact->setText(parts[3]);
	}
	ui->detail_exon->setText(parts[4].mid(4));
	ui->detail_cdna->setText(parts[5]);
	ui->detail_protein->setText(parts[6]);

	//enable next button if more than one transcript
	ui->trans_prev->setEnabled(trans_data.count()>1 && index>0);
	ui->trans_next->setEnabled(trans_data.count()>1 && index<trans_data.count()-1);
}

void VariantDetailsDockWidget::nextTanscript()
{
	setTranscript(trans_curr+1);
}

void VariantDetailsDockWidget::previousTanscript()
{
	setTranscript(trans_curr-1);
}

QString VariantDetailsDockWidget::formatLink(QString text, QString url, Color bgcolor)
{
	return "<a style=\"color: #000000; background-color:" + colorToString(bgcolor) + ";\" href=\"" + url + "\">" + text + "</a>";
}

QString VariantDetailsDockWidget::formatText(QString text, Color bgcolor)
{
	return "<span style=\"background-color: " + colorToString(bgcolor) + ";\">" + text + "</span>";
}

QList<VariantDetailsDockWidget::DBEntry> VariantDetailsDockWidget::parseDB(QString anno)
{
	QList<DBEntry> output;

	QStringList entries = anno.split("];");
	foreach(QString entry, entries)
	{
		entry = entry.trimmed();
		if (entry.isEmpty()) continue;

		int sep_index = entry.indexOf("[");
		output.append(DBEntry{entry.left(sep_index-1), entry.mid(sep_index+1)});
	}

	return output;
}

QString VariantDetailsDockWidget::nobr()
{
	return "<p style='white-space:pre; margin:0; padding:0;'>";
}
