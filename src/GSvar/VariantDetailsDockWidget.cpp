#include "VariantDetailsDockWidget.h"
#include "ui_VariantDetailsDockWidget.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QPixmap>
#include <QMessageBox>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QUrl>
#include <QMenu>
#include "Settings.h"
#include "NGSD.h"
#include "Log.h"
#include "HttpHandler.h"
#include "GSvarHelper.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include <QHeaderView>

VariantDetailsDockWidget::VariantDetailsDockWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::VariantDetailsDockWidget)
{
	ui->setupUi(this);

	//signals + slots
	connect(ui->trans_prev, SIGNAL(clicked(bool)), this, SLOT(previousTanscript()));
	connect(ui->trans_next, SIGNAL(clicked(bool)), this, SLOT(nextTanscript()));
	connect(ui->variant, SIGNAL(linkActivated(QString)), this, SLOT(variantClicked(QString)));
	connect(ui->gnomad, SIGNAL(linkActivated(QString)), this, SLOT(gnomadClicked(QString)));
	connect(ui->var_btn, SIGNAL(clicked(bool)), this, SLOT(variantButtonClicked()));
	connect(ui->trans, SIGNAL(linkActivated(QString)), this, SLOT(transcriptClicked(QString)));

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
	}

	//reset
	clear();
}

VariantDetailsDockWidget::~VariantDetailsDockWidget()
{
	delete ui;
}

void VariantDetailsDockWidget::setLabelTooltips(const VariantList& vl)
{
	//general
	ui->label_quality->setToolTip(vl.annotationDescriptionByName("quality").description());
	ui->label_filter->setToolTip(vl.annotationDescriptionByName("filter").description());

	//DBs
	ui->label_dbsnp->setToolTip(vl.annotationDescriptionByName("dbSNP").description());
	ui->label_clinvar->setToolTip(vl.annotationDescriptionByName("ClinVar").description());
	ui->label_hgmd->setToolTip(vl.annotationDescriptionByName("HGMD", false).description()); //optional
	ui->label_omim->setToolTip(vl.annotationDescriptionByName("OMIM", false).description()); //optional
	ui->label_cosmic->setToolTip(vl.annotationDescriptionByName("COSMIC").description());

	//AFs
	ui->label_tg->setToolTip(vl.annotationDescriptionByName("1000g").description());
	ui->label_gnomad->setToolTip(vl.annotationDescriptionByName("gnomAD").description());
	ui->label_gnomad_hom_hemi->setToolTip(vl.annotationDescriptionByName("gnomAD_hom_hemi").description());
	ui->label_gnomad_sub->setToolTip(vl.annotationDescriptionByName("gnomAD_sub").description());

	//pathogenicity predictions
	ui->label_phylop->setToolTip(vl.annotationDescriptionByName("phyloP").description());
	ui->label_sift->setToolTip(vl.annotationDescriptionByName("Sift").description());
	ui->label_polyphen->setToolTip(vl.annotationDescriptionByName("PolyPhen").description());
	ui->label_cadd->setToolTip(vl.annotationDescriptionByName("CADD").description());
	ui->label_fathmm->setToolTip(vl.annotationDescriptionByName("fathmm-MKL").description());
	ui->label_revel->setToolTip(vl.annotationDescriptionByName("REVEL").description());

	//splicing/regulatory
	ui->label_maxentscan->setToolTip(vl.annotationDescriptionByName("MaxEntScan").description());
	ui->label_genesplicer->setToolTip(vl.annotationDescriptionByName("GeneSplicer").description());
	ui->label_dbscsnv->setToolTip(vl.annotationDescriptionByName("dbscSNV").description());
	ui->label_mmsplice_deltaLogitPsi->setToolTip(vl.annotationDescriptionByName("MMSplice_DeltaLogitPSI", false).description());
	ui->label_mmsplice_pathogenicity->setToolTip(vl.annotationDescriptionByName("MMSplice_pathogenicity", false).description());
	ui->label_regulatory->setToolTip(vl.annotationDescriptionByName("regulatory", false).description());
	ui->label_mmsplice->setToolTip("MMSplice prediction of splice-site variations.");

	//NGSD (all optional)
	ui->label_ngsd_class->setToolTip(vl.annotationDescriptionByName("classification", false).description());
	ui->label_ngsd_count->setToolTip("Homozygous / heterozygous variant count in NGSD.");
	ui->label_ngsd_group->setToolTip(vl.annotationDescriptionByName("NGSD_group", false).description());
	ui->label_ngsd_comment->setToolTip(vl.annotationDescriptionByName("comment", false).description());
	ui->label_ngsd_validation->setToolTip(vl.annotationDescriptionByName("validation", false).description());
}

void VariantDetailsDockWidget::updateVariant(const VariantList& vl, int index)
{
	//determine genotype column index
	int geno_i = -1;
	AnalysisType type = vl.type();
	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO)
	{
		geno_i = vl.getSampleHeader().infoByStatus(true).column_index;
	}

	//variant
	QString variant = vl[index].toString(false, 10);
	variant_str = vl[index].toString();
	if(geno_i!=-1)
	{
		variant += " (" + vl[index].annotations()[geno_i] + ")";
	}
	ui->variant->setText(formatLink(variant, variant));

	//closeby variant warning
	QStringList closeby;
	BedLine range(vl[index].chr(), vl[index].start()-2, vl[index].end()+2);
	for (int i=std::max(0, index-10); i<std::min(vl.count(), index+10); ++i)
	{
		if (i==index) continue;
		if (vl[i].overlapsWith(range))
		{
			QString variant = vl[i].toString();
			if (geno_i!=-1)
			{
				variant += " (" + vl[i].annotations()[geno_i] + ")";
			}
			closeby.append(variant);
		}
	}
	ui->warn_closeby->setPixmap(QPixmap(":/Icons/Attention.png"));
	ui->warn_closeby->setVisible(closeby.count()>0);
	ui->warn_closeby->setToolTip("Nearby variant(s):\n" + closeby.join("\n"));

	//details
	initTranscriptDetails(vl, index);

	//base information
	setAnnotation(ui->gene, vl, index, "gene_info");
	setAnnotation(ui->quality, vl, index, "quality");
	setAnnotation(ui->filter, vl, index, "filter");

	//public databases
	setAnnotation(ui->dbsnp, vl, index, "dbSNP");
	setAnnotation(ui->clinvar, vl, index, "ClinVar");
	setAnnotation(ui->hgmd, vl, index, "HGMD");
	setAnnotation(ui->omim, vl, index, "OMIM");
	setAnnotation(ui->cosmic, vl, index, "COSMIC");

	//public allel frequencies
	setAnnotation(ui->tg, vl, index, "1000g");
	setAnnotation(ui->gnomad, vl, index, "gnomAD");
	setAnnotation(ui->gnomad_hom_hemi, vl, index, "gnomAD_hom_hemi");
	setAnnotation(ui->gnomad_sub, vl, index, "gnomAD_sub");

	//pathogenity predictions
	setAnnotation(ui->phylop, vl, index, "phyloP");
	setAnnotation(ui->sift, vl, index, "Sift");
	setAnnotation(ui->polyphen, vl, index, "PolyPhen");
	setAnnotation(ui->cadd, vl, index, "CADD");
	setAnnotation(ui->fathmm, vl, index, "fathmm-MKL");
	setAnnotation(ui->revel, vl, index, "REVEL");

	//splicing/regulatory
	setAnnotation(ui->maxentscan, vl, index, "MaxEntScan");
	setAnnotation(ui->genesplicer, vl, index, "GeneSplicer");
	setAnnotation(ui->dbscsnv, vl, index, "dbscSNV");
	setAnnotation(ui->mmsplice_deltaLogitPsi, vl, index, "MMSplice_DeltaLogitPSI");
	setAnnotation(ui->mmsplice_pathogenicity, vl, index, "MMSplice_pathogenicity");
	setAnnotation(ui->regulatory, vl, index, "regulatory");

	//NGSD
	setAnnotation(ui->ngsd_class, vl, index, "classification");
	QString ngsd_count = "";
	int hom_index = vl.annotationIndexByName("NGSD_hom", true, false);
	int het_index = vl.annotationIndexByName("NGSD_het", true, false);
	if(hom_index!=-1 && het_index!=-1)
	{
		QString hom = vl[index].annotations()[hom_index];
		QString het = vl[index].annotations()[het_index];
		if (hom.startsWith("n/a") && het.startsWith("n/a"))
		{
			ngsd_count = hom;
		}
		else
		{
			ngsd_count = hom + " / " + het;
		}
	}
	ui->ngsd_count->setText(ngsd_count);
	setAnnotation(ui->ngsd_group, vl, index, "NGSD_group");
	setAnnotation(ui->ngsd_comment, vl, index, "comment");
	setAnnotation(ui->ngsd_validation, vl, index, "validation");

	//somatic details
	setAnnotation(ui->somatic_cgi_driver_status, vl, index, "CGI_driver_statement");
	setAnnotation(ui->somatic_cgi_gene_role, vl, index , "CGI_gene_role");
	setAnnotation(ui->somatic_classification, vl, index, "somatic_classification");
	setAnnotation(ui->somatic_count, vl, index, "NGSD_som_c");
	setAnnotation(ui->somatic_oncogene, vl, index, "ncg_oncogene");
	setAnnotation(ui->somatic_tsg, vl, index, "ncg_tsg");

	//update NGSD button
	ui->var_btn->setEnabled(LoginManager::active());
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
	variant_str.clear();
	ui->variant->setText("No variant or several variants selected");
	ui->warn_closeby->setVisible(false);

	//details
	ui->trans->setText("<span style=\"font-weight:600; color:#222222;\">&nbsp;<span>"); //bold => higher
	ui->trans_prev->setEnabled(false);
	ui->trans_next->setEnabled(false);

	//edit button
	ui->var_btn->setEnabled(false);
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
			QStringList rs_numbers = anno.replace("rs", "").split(',');
			foreach(QString rs_number, rs_numbers)
			{
				if (!rs_number.trimmed().isEmpty())
				{
					text += formatLink("rs"+rs_number, "http://www.ncbi.nlm.nih.gov/projects/SNP/snp_ref.cgi?rs=" + rs_number) + " ";
				}
			}
		}
		else if(name=="OMIM")
		{
			foreach(const DBEntry& entry, parseDB(anno, ','))
			{
				text += formatLink(entry.id, "http://omim.org/entry/" + entry.id) + " ";
				tooltip += nobr() + entry.id + ": " + entry.details;
			}
		}
		else if(name=="ClinVar")
		{
			foreach(const DBEntry& entry, parseDB(anno, ';'))
			{
				//determine color
				Color color = NONE;
				if (entry.details.contains("conflicting interpretations of pathogenicity")) color = NONE;
				else if (entry.details.contains("likely pathogenic")) color = ORANGE;
				else if (entry.details.contains("pathogenic")) color = RED;
				else if (entry.details.contains("benign")) color = GREEN;

				QString url = entry.id.startsWith("RCV") ? "http://www.ncbi.nlm.nih.gov/clinvar/" : "http://www.ncbi.nlm.nih.gov/clinvar?term=";
				text += formatLink(entry.id, url + entry.id, color) + " ";
				tooltip += nobr() + entry.id + ": " + entry.details;
			}
		}
		else if(name=="HGMD")
		{
			foreach(const DBEntry& entry, parseDB(anno, ';'))
			{
				//determine color
				Color color = NONE;
				if (entry.details.contains("CLASS=DM?")) color = ORANGE;
				else if (entry.details.contains("CLASS=DM")) color = RED;
				else if (entry.details.contains("CLASS=DP") || entry.details.contains("CLASS=DFP")) color = ORANGE;

				text += formatLink(entry.id, "https://portal.biobase-international.com/hgmd/pro/mut.php?acc=" + entry.id, color) + " ";
				tooltip += nobr() + entry.id + ": " + entry.details;
			}
		}
		else if(name=="COSMIC")
		{
			QStringList ids = anno.split(",");
			foreach(QString id, ids)
			{
				QString temp_id = id.mid(4).trimmed();
				if(temp_id.isEmpty()) continue;

				if(id.startsWith("COSM"))
				{
					text += formatLink(temp_id, "https://cancer.sanger.ac.uk/cosmic/mutation/overview?id=" + temp_id) + " ";
				}
				else if(id.startsWith("COSN"))
				{
					text += formatLink(temp_id, "https://cancer.sanger.ac.uk/cosmic/ncv/overview?id=" + temp_id) + " ";
				}
				else if(id.startsWith("COSV"))
				{
					text += formatLink(temp_id, "https://cancer.sanger.ac.uk/cosmic/search?q=COSV" + temp_id) + " ";
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
		else if(name=="CADD")
		{
			bool ok = true;
			double value = anno.toDouble(&ok);
			if (ok && value>=20)
			{
				text = formatText(anno, RED);
			}
			else if (ok && value>=15)
			{
				text = formatText(anno, ORANGE);
			}
			else
			{
				text = anno;
			}
		}
		else if(name=="Sift" || name=="PolyPhen")
		{
			text = anno.replace("D", formatText("D", RED)).replace("P", formatText("P", ORANGE));
		}
		else if(name=="fathmm-MKL")
		{
			double max = 0.0;
			QStringList parts = anno.split(",");
			foreach(QString part, parts)
			{
				bool ok = true;
				double value = part.toDouble(&ok);
				if (ok) max = std::max(value, max);
			}

			if (max>=0.9)
			{
				text = formatText(anno, RED);
			}
			else if (max>=0.5)
			{
				text = formatText(anno, ORANGE);
			}
			else
			{
				text = anno;
			}
		}
		else if(name=="REVEL")
		{
			bool ok = true;
			double value = anno.toDouble(&ok);
			if (ok && value>=0.9)
			{
				text = formatText(anno, RED);
			}
			else if (ok && value>=0.5)
			{
				text = formatText(anno, ORANGE);
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
				text = formatText("likely pathogenic (4)", RED);
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
		else if(name=="1000g" || name=="gnomAD")
		{
			if (anno=="")
			{
				text = "n/a";
			}
			else
			{
				bool ok = true;
				double value = anno.toDouble(&ok);
				text = (ok && value>=0.05) ? formatText(anno, GREEN) : anno;
			}

			//make gnomAD value clickable
			if(name=="gnomAD")
			{
				text = formatLink(text, vl[index].toString(true));
			}
		}
		else if (name=="gnomAD_sub")
		{
			if (anno=="")
			{
				text = "n/a";
			}
			else
			{
				bool high_af = false;
				QStringList parts = anno.split(",");
				foreach(QString part, parts)
				{
					bool ok = true;
					double value = part.toDouble(&ok);
					if (ok && value>=0.05)
					{
						high_af = true;
					}
				}
				text = high_af ? formatText(anno, GREEN) : anno;
			}
		}
		else if(name=="comment")
		{
			text = anno;
			tooltip = anno;
		}
		else  if(name == "CGI_driver_statement")
		{
			if(anno.contains("known"))
			{
				text = formatText("driver (known)", RED);
				tooltip = anno.replace("known in:","").replace(";",", ");
			}
			else if(anno.contains("predicted driver")) text = formatText("driver (predicted)", RED);
			else if(anno.contains("predicted passenger")) text = "passenger (predicted)";
			else text = anno;
		}
		else if(name=="ncg_oncogene")
		{
			if(anno.contains("1")) text = formatText(anno,RED);
			else text = anno;
		}
		else if(name=="ncg_tsg")
		{
			if(anno.contains("1")) text = formatText(anno, RED);
			else text = anno;
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

double VariantDetailsDockWidget::maxAlleleFrequency(const VariantList& vl, int index) const
{
	double output = 0.0;

	bool ok;
	double value;

	int idx = vl.annotationIndexByName("1000g", true, false);
	if (idx!=-1)
	{
		value = vl[index].annotations()[idx].toDouble(&ok);
		if (ok)
		{
			output = std::max(output, value);
		}
	}

	idx = vl.annotationIndexByName("gnomAD", true, false);
	if (idx!=-1)
	{
		value = vl[index].annotations()[idx].toDouble(&ok);
		if (ok)
		{
			output = std::max(output, value);
		}
	}

	return output;
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
    const QMap<QByteArray, QByteArrayList>& preferred_transcripts = GSvarHelper::preferredTranscripts();
	try
	{
		trans_data = vl[index].transcriptAnnotations(a_index);
	}
	catch(ProgrammingException)
	{
		trans_data.clear();
	}

	//select transcript
	if (trans_data.isEmpty())
	{
		ui->detail_type->clear();
		ui->detail_impact->clear();
		ui->detail_exon->clear();
		ui->detail_cdna->clear();
		ui->detail_protein->clear();
		ui->detail_domain->clear();
	}
	else
	{
		//use first preferred transcript
		for (int i=0; i<trans_data.count(); ++i)
		{
			if (preferred_transcripts.value(trans_data[i].gene).contains(trans_data[i].id))
			{
				setTranscript(i);
				break;
			}
		}
		//use first high-impact transcript
		if (trans_curr==-1)
		{
			for (int i=0; i<trans_data.count(); ++i)
			{
				if (trans_data[i].impact=="HIGH")
				{
					setTranscript(i);
					break;
				}
			}
		}
		//fallback: first transcript
		if (trans_curr==-1)
		{
			setTranscript(0);
		}
	}

	//tooltip if more than one transcript
	QString tooltip;
	foreach(const VariantTranscript& trans, trans_data)
	{
		//highlight preferred transcripts
		bool is_pt = preferred_transcripts.value(trans.gene).contains(trans.id);
		tooltip += nobr() + (is_pt ? "<b>" : "") + trans.toString(' ') + (is_pt ? "</b>" : "");
	}
	ui->trans->setToolTip(tooltip);
}

void VariantDetailsDockWidget::setTranscript(int index)
{
	trans_curr = index;
	const VariantTranscript& trans = trans_data[index];

	//set transcript label
	QString text = formatLink(trans.gene, trans.gene) + " " + formatLink(trans.id, "http://grch37.ensembl.org/Homo_sapiens/Transcript/Summary?t=" + trans.id);
	if (trans_data.count()>1)
	{
		text += " (" + QString::number(index+1) + "/" + QString::number(trans_data.count()) + ")";
	}
	ui->trans->setText("<span style=\"font-weight:600; color:#222222;\">" + text + "<span>");

	//set detail labels
	if (trans.impact=="HIGH")
	{
		ui->detail_type->setText(formatText(trans.type, RED));
		ui->detail_impact->setText(formatText(trans.impact, RED));
	}
	else
	{
		ui->detail_type->setText(trans.type);
		ui->detail_impact->setText(trans.impact);
	}
	QString exon_nr = trans.exon;
	if (exon_nr.startsWith("exon"))
	{
		exon_nr.replace("exon", "");
	}
	if (exon_nr.startsWith("intron"))
	{
		exon_nr.replace("intron", "");
		exon_nr += " (intron)";
	}
	ui->detail_exon->setText(exon_nr);
	ui->detail_cdna->setText(trans.hgvs_c);
	ui->detail_protein->setText(trans.hgvs_p);
	text = trans.domain;
	if (text!="")
	{
		// check if domain contains only Pfam Id (old) or additional description
		QStringList split_domain_string = text.split(" [");
		if (split_domain_string.size() > 1)
		{
			ui->detail_domain->setToolTip(split_domain_string[1].replace("]", ""));
		}
		text = formatLink(split_domain_string[0], "https://pfam.xfam.org/family/" + split_domain_string[0]);
	}
	ui->detail_domain->setText(text);

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

void VariantDetailsDockWidget::variantClicked(QString link)
{
	//extract location only
	link = link.left(link.indexOf(' '));

	emit jumbToRegion(link);
}

QString VariantDetailsDockWidget::formatLink(QString text, QString url, Color bgcolor)
{
	return "<a style=\"color: #000000; background-color:" + colorToString(bgcolor) + ";\" href=\"" + url + "\">" + text + "</a>";
}

QString VariantDetailsDockWidget::formatText(QString text, Color bgcolor)
{
	return "<span style=\"background-color: " + colorToString(bgcolor) + ";\">" + text + "</span>";
}

QList<VariantDetailsDockWidget::DBEntry> VariantDetailsDockWidget::parseDB(QString anno, char sep)
{
	QList<DBEntry> output;

	anno = anno.trimmed();
	if (anno.endsWith("]")) anno.chop(1);

	QStringList entries = anno.split(QString("]") + sep);
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

void VariantDetailsDockWidget::gnomadClicked(QString link)
{
	QStringList parts = link.split(' ');
	Chromosome chr(parts[0]);
	QString start = parts[1];
	QString ref = parts[3];
	QString obs = parts[4];

	QString url;
	if (obs=="-") //deletion
	{
		int pos = start.toInt()-1;
		FastaFileIndex idx(Settings::string("reference_genome"));
		QString base = idx.seq(chr, pos, 1);
		url = chr.strNormalized(false) + "-" + QString::number(pos) + "-" + base + ref + "-" + base;
	}
	else if (ref=="-") //insertion
	{
		int pos = start.toInt();
		FastaFileIndex idx(Settings::string("reference_genome"));
		QString base = idx.seq(chr, pos, 1);
		url = chr.strNormalized(false) + "-" + start + "-" + base + "-" + base + obs;
	}
	else //snv
	{
		url =  chr.strNormalized(false) + "-" + start + "-" + ref + "-" + obs;
	}
	QDesktopServices::openUrl(QUrl("http://gnomad.broadinstitute.org/variant/" + url));
}

void VariantDetailsDockWidget::transcriptClicked(QString link)
{
	if (link.startsWith("http")) //transcript
	{
		QDesktopServices::openUrl(QUrl(link));
	}
	else //gene
	{
		emit openGeneTab(link);
	}
}

void VariantDetailsDockWidget::variantButtonClicked()
{
	if (variant_str.isEmpty()) return;

	emit openVariantTab(Variant::fromString(variant_str));
}

QList<KeyValuePair> VariantDetailsDockWidget::DBEntry::splitByName() const
{
	QList<KeyValuePair> output;

	//determine key start/end
	QList<QPair<int, int>> hits;
	QRegularExpression regexp("[A-Z0-9_-]+=");
	QRegularExpressionMatchIterator i = regexp.globalMatch(details);
	while (i.hasNext())
	{
		QRegularExpressionMatch match = i.next();
		hits << QPair<int, int>(match.capturedStart(0), match.capturedEnd());
	}
	hits << QPair<int, int>(details.count(), -1);

	for (int i=0; i<hits.count()-1; ++i)
	{
		QString key = details.mid(hits[i].first, hits[i].second-hits[i].first-1).trimmed();
		QString value = details.mid(hits[i].second, hits[i+1].first-hits[i].second).trimmed();
		if (value.startsWith('"') && value.endsWith('"')) value = value.mid(1, value.length()-2);
		output << KeyValuePair(key, value);
	}

	return output;
}

void VariantDetailsDockWidget::showOverviewTable(QString title, QString text, char sep, QByteArray url_prefix)
{
	//determine headers
	QList<DBEntry> entries = parseDB(text, sep);
	QStringList headers;
	headers << "ID";
	foreach(const DBEntry& entry, entries)
	{
		QList<KeyValuePair> parts = entry.splitByName();
		foreach(const KeyValuePair& pair, parts)
		{
			if (!headers.contains(pair.key))
			{
				headers << pair.key;
			}
		}
	}

	//set up table widget
	QTableWidget* table = new QTableWidget();
	table->setRowCount(entries.count());
	table->setColumnCount(headers.count());
	table->setHorizontalHeaderLabels(headers);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setAlternatingRowColors(true);
	table->setWordWrap(false);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
	table->setSelectionBehavior(QAbstractItemView::SelectItems);
	table->verticalHeader()->setVisible(false);
	int row = 0;
	foreach(const DBEntry& entry, entries)
	{
		QString id = entry.id.trimmed();
		if (!url_prefix.isEmpty()) //URL
		{
			QLabel* label = GUIHelper::createLinkLabel("<a href='" + url_prefix + id + "'>" + id + "</a>");
			table->setCellWidget(row, 0, label);
		}
		else
		{
			table->setItem(row, 0, new QTableWidgetItem(id));
		}

		QList<KeyValuePair> parts = entry.splitByName();
		foreach(const KeyValuePair& pair, parts)
		{
			int col = headers.indexOf(pair.key);

			QString text = pair.value.trimmed();
			if (text.startsWith("http://") || text.startsWith("https://")) //URL
			{
				QLabel* label = GUIHelper::createLinkLabel("<a href='" + text + "'>" + text + "</a>");
				table->setCellWidget(row, col, label);
			}
			else
			{
				table->setItem(row, col, new QTableWidgetItem(text));
			}
		}

		++row;
	}

	//show table
	auto dlg = GUIHelper::createDialog(table, title);
	dlg->setMinimumSize(1200, 800);
	GUIHelper::resizeTableCells(table);
	dlg->exec();

	//delete
	delete table;
}
