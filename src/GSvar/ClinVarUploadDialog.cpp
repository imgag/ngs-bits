#include "ClinVarUploadDialog.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPrinter>
#include <QPrintDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>

ClinvarUploadDialog::ClinvarUploadDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	initGui();
}

void ClinvarUploadDialog::setData(ClinvarUploadData data)
{
	//sample data
	ui_.processed_sample->setText(data.processed_sample);

	ui_.processed_sample->setEnabled(false);
	ui_.gender->setEnabled(false);

	//variant data (variant 1)
	variant1_ = data.variant;
	QByteArray chr = data.variant.chr().str();
	if (chr=="chrMT") chr = "chrM";
	ui_.cb_chr1->setCurrentText(chr);
	ui_.cb_chr1->setEnabled(false);
//	ui_.classification->setEnabled(false);
//	ui_.genotype->setEnabled(false);

	//variant data (variant 2)
	if(data.variant2.isValid())
	{
		variant2_ = data.variant2;
//		ui_.hgvs_g2->setText(data.hgvs_g2);
//		ui_.hgvs_c2->setText(data.hgvs_c2);
//		ui_.hgvs_p2->setText(data.hgvs_p2);
//		ui_.classification2->setCurrentText(data.classification2);
//		ui_.genotype2->setCurrentText(data.genotype2);


//		ui_.hgvs_g2->setEnabled(true);
//		ui_.hgvs_c2->setEnabled(true);
//		ui_.hgvs_p2->setEnabled(true);
//		ui_.refseq_btn2->setEnabled(true);
	}

//	ui_.genotype2->setEnabled(false);
//	ui_.classification2->setEnabled(false);

	//if not in free mode => GUI of 2nd variant needs no updates
	disconnect(this, SLOT(updateSecondVariantGui()));

	//phenotype data
	ui_.phenos->setPhenotypes(data.phenos);

    data_ = data;
}

void ClinvarUploadDialog::initGui()
{
    // set combobox entries
    ui_.cb_clin_sig_desc->addItems(CLINICAL_SIGNIFICANCE_DESCRIPTION);
    ui_.cb_inheritance->addItems(MODE_OF_INHERITANCE);
    ui_.cb_affected_status->addItems(AFFECTED_STATUS);
    ui_.cb_allele_origin->addItems(ALLELE_ORIGIN);
    ui_.cb_collection_method->addItems(COLLECTION_METHOD);
    ui_.cb_method_type->addItems(STRUCT_VAR_METHOD_TYPE);
    ui_.cb_chr1->addItems(CHR);
    ui_.cb_chr2->addItems(CHR);
    ui_.cb_var_type1->addItems(VARIANT_TYPE);
    ui_.cb_var_type2->addItems(VARIANT_TYPE);





    //connect signal and slots
    connect(ui_.upload_btn, SIGNAL(clicked(bool)), this, SLOT(upload()));

    connect(ui_.processed_sample, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
    connect(ui_.cb_chr1, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.gene, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.nm_number, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.hgvs_c, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.hgvs_g, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.hgvs_p, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.genotype, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.classification, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.hgvs_c2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.hgvs_g2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.hgvs_p2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.genotype2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
//	connect(ui_.classification2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
    connect(ui_.phenos, SIGNAL(phenotypeSelectionChanged()), this, SLOT(checkGuiData()));

    //update GUI elements for 2nd variant (only for free mode)
//	connect(ui_.genotype, SIGNAL(currentTextChanged(QString)), this, SLOT(updateSecondVariantGui()));
//	connect(ui_.genotype2, SIGNAL(currentTextChanged(QString)), this, SLOT(updateSecondVariantGui()));

    connect(ui_.print_btn, SIGNAL(clicked(bool)), this, SLOT(printResults()));
    connect(ui_.comment_upload, SIGNAL(textChanged()), this, SLOT(updatePrintButton()));
}

void ClinvarUploadDialog::upload()
{
	QJsonObject json = createJson();

	//TODO: remove
	// write to file
	qDebug()<<"Write to file!";
	QJsonDocument json_doc = QJsonDocument(json);
	QFile json_file("clinvar_submission.json");
	json_file.open(QFile::WriteOnly);
	json_file.write(json_doc.toJson());
	json_file.close();
}

void ClinvarUploadDialog::checkGuiData()
{
	//check if already published
	if (ui_.processed_sample->text()!="" && variant1_.isValid())
	{
		QString upload_details = db_.getVariantPublication(ui_.processed_sample->text(), variant1_);
		if (upload_details!="")
		{
			ui_.upload_btn->setEnabled(false);
			ui_.comment_upload->setText("<font color='red'>ERROR: variant already uploaded!</font><br>" + upload_details);
			return;
		}
	}

	//perform checks
	QStringList errors;
	if (ui_.processed_sample->text().trimmed().isEmpty())
	{
		errors << "Processed sample unset!";
	}
	if (ui_.cb_chr1->currentText().trimmed().isEmpty())
	{
		errors << "Chromosome unset!";
	}
//	if (ui_.gene->text().trimmed().isEmpty())
//	{
//		errors << "Gene unset!";
//	}
//	if (ui_.nm_number->text().trimmed().isEmpty())
//	{
//		errors << "Transcript unset!";
//	}
//	if (ui_.hgvs_g->text().trimmed().isEmpty())
//	{
//		errors << "HGVS.g unset!";
//	}
//	if (!ui_.hgvs_g->text().trimmed().isEmpty() && !ui_.hgvs_g->text().trimmed().startsWith("g."))
//	{
//		errors << "HGVS.g must start with 'g.'!";
//	}
//	if (ui_.hgvs_c->text().trimmed().isEmpty())
//	{
//		errors << "HGVS.c unset!";
//	}
//	if (!ui_.hgvs_c->text().trimmed().isEmpty() && !ui_.hgvs_c->text().trimmed().startsWith("c."))
//	{
//		errors << "HGVS.c must start with 'c.'!";
//	}
//	if (!ui_.hgvs_p->text().trimmed().isEmpty() && !ui_.hgvs_p->text().trimmed().startsWith("p."))
//	{
//		errors << "HGVS.p must start with 'p.'!";
//	}
//	if (ui_.genotype->currentText().trimmed().isEmpty())
//	{
//		errors << "Genotype unset!";
//	}
//	if (ui_.classification->currentText().trimmed().isEmpty())
//	{
//		errors << "Classification unset!";
//	}

//	if (isCompHet())
//	{
//		if (ui_.hgvs_g2->text().trimmed().isEmpty())
//		{
//			errors << "HGVS.g unset (variant 2)!";
//		}
//		if (!ui_.hgvs_g2->text().trimmed().isEmpty() && !ui_.hgvs_g2->text().trimmed().startsWith("g."))
//		{
//			errors << "HGVS.g must start with 'g.' (variant 2)!";
//		}
//		if (ui_.hgvs_c2->text().trimmed().isEmpty())
//		{
//			errors << "HGVS.c unset (variant 2)!";
//		}
//		if (!ui_.hgvs_c2->text().trimmed().isEmpty() && !ui_.hgvs_c2->text().trimmed().startsWith("c."))
//		{
//			errors << "HGVS.c must start with 'c.' (variant 2)!";
//		}
//		if (!ui_.hgvs_p2->text().trimmed().isEmpty() && !ui_.hgvs_p2->text().trimmed().startsWith("p."))
//		{
//			errors << "HGVS.p must start with 'p.' (variant 2)!";
//		}
//		if (ui_.genotype->currentText()!="het" || ui_.genotype2->currentText()!="het")
//		{
//			errors << "Two variants for upload, but they are not compound-heterozygous!";
//		}
//		if (ui_.classification2->currentText().trimmed().isEmpty())
//		{
//			errors << "Classification unset (variant 2)!";
//		}
//	}

	if (ui_.phenos->selectedPhenotypes().count()==0)
	{
		errors << "No phenotypes selected!";
	}

	//show error or enable upload button
	if (errors.count()>0)
	{
		ui_.upload_btn->setEnabled(false);
		ui_.comment_upload->setText("Cannot upload data because:\n  - " +  errors.join("\n  - "));
	}
	else
	{
		ui_.upload_btn->setEnabled(true);
		ui_.comment_upload->clear();
	}
}

void ClinvarUploadDialog::printResults()
{
	QPrinter printer(QPrinter::HighResolution);
	printer.setFullPage(true);
	QPrintDialog *dlg = new QPrintDialog(&printer, this);
	if (dlg->exec() == QDialog::Accepted)
	{
		QTextDocument *doc = new QTextDocument();
		doc->setPlainText(ui_.comment_upload->toPlainText());
		doc->print(&printer);
		delete doc;
	}
	delete dlg;
}

void ClinvarUploadDialog::updatePrintButton()
{
	ui_.print_btn->setEnabled(!ui_.comment_upload->toPlainText().trimmed().isEmpty());
}

void ClinvarUploadDialog::updateSecondVariantGui()
{
//	bool enabled = ui_.genotype->currentText()=="het" && ui_.genotype2->currentText()=="het";

//	ui_.hgvs_g2->setEnabled(enabled);
//	ui_.hgvs_c2->setEnabled(enabled);
//	ui_.hgvs_p2->setEnabled(enabled);
//	ui_.classification2->setEnabled(enabled);
//	ui_.refseq_btn2->setEnabled(enabled);
}

void ClinvarUploadDialog::setTranscriptInfoVariant1()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr) THROW(ProgrammingException, "This should not happen!");

	bool ok = false;
	int index = action->data().toInt(&ok);
	if (!ok) THROW(ProgrammingException, "This should not happen!");
//	ui_.gene->setText(trans.gene);
//	ui_.nm_number->setText(trans.id);
//	ui_.hgvs_c->setText(trans.hgvs_c);
//	ui_.hgvs_p->setText(trans.hgvs_p);

	checkGuiData();
}

void ClinvarUploadDialog::setTranscriptInfoVariant2()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr) THROW(ProgrammingException, "This should not happen!");

	bool ok = false;
	int index = action->data().toInt(&ok);
	if (!ok) THROW(ProgrammingException, "This should not happen!");

//	if (trans.id!=ui_.nm_number->text())
//	{
//		QMessageBox::warning(this, "Transcript mismatch error", ui_.nm_number->text() + " selected as transcript for variant 1.\n" + trans.id + " selected as transcript for variant 2.\n\nThey do not match!");
//		return;
//	}

//	ui_.hgvs_c2->setText(trans.hgvs_c);
//	ui_.hgvs_p2->setText(trans.hgvs_p);

	checkGuiData();
}

bool ClinvarUploadDialog::isCompHet() const
{
//	return variant2_.isValid() || ui_.genotype2->currentText()=="het";
    return false;
}

QJsonObject ClinvarUploadDialog::createJson()
{
    QJsonObject json;

    //TODO: Check GUI for correct entries

    //optional
    json.insert("behalfOrgID", "");

    //required
    QJsonObject clinvar_submission;
    {
        //optional
        QJsonObject assertion_criteria;
        {

        }
        clinvar_submission.insert("assertionCriteria", assertion_criteria);

        //required
        QJsonObject clinical_significance;
        {
            //optional
            clinical_significance.insert("citation", QJsonArray());

            //required
            clinical_significance.insert("clinicalSignificanceDescription", ui_.cb_clin_sig_desc->currentText());

            //optional
            clinical_significance.insert("comment", ui_.le_clin_sig_desc_comment->text());

            //optional
            clinical_significance.insert("dateLastEvaluated", ui_.de_last_eval->date().toString("yyyy-MM-dd"));

            //optional
            clinical_significance.insert("modeOfInheritance", ui_.cb_inheritance->currentText());
        }
        clinvar_submission.insert("clinicalSignificance", clinical_significance);

        //optional
        clinvar_submission.insert("clinvarAccession", "");

        //required
        QJsonObject condition_set;
        {
            QJsonArray condition;
            {
                //TODO: parse HPO terms
            }
            condition_set.insert("condition", condition);
        }
        clinvar_submission.insert("conditionSet", condition_set);

        //optional
        clinvar_submission.insert("localID", ui_.le_local_id->text());

        //optional
        clinvar_submission.insert("localKey", ui_.le_local_key->text());

        //required
        QJsonObject observed_in;
        {
            //required
            observed_in.insert("affectedStatus", ui_.cb_affected_status->currentText());

            //required
            observed_in.insert("alleleOrigin", ui_.cb_allele_origin->currentText());

            //optional
            QJsonArray clinical_features;
            {

            }
            observed_in.insert("clinicalFeatures", clinical_features);

            //optional
            observed_in.insert("clinicalFeaturesComment", ui_.le_clin_feat_comment->text());

            //required
            observed_in.insert("collectionMethod", ui_.cb_collection_method->currentText());

            //optional
            observed_in.insert("numberOfIndividuals", ui_.sb_n_individuals->value());

            //optional
            observed_in.insert("structVarMethodType", ui_.cb_method_type->currentText());

        }
        clinvar_submission.insert("observedIn", QJsonArray() << observed_in);

        //required
        clinvar_submission.insert("recordStatus", ui_.cb_record_status->currentText());

        //required
        clinvar_submission.insert("releaseStatus", ui_.cb_release_status->currentText());

        //required
        QJsonObject variant_set;
        {
            QJsonArray variants;
            {
                //1. variant
                QJsonObject variant1;
                {
                    //required (except hgvs)
                    QJsonObject chromosome_coordinates;
                    {
                     chromosome_coordinates.insert("alternateAllele", ui_.le_alt1->text());
                     chromosome_coordinates.insert("assembly", ui_.cb_assembly->currentText());
                     chromosome_coordinates.insert("chromosome", ui_.cb_chr1->currentText());
                     chromosome_coordinates.insert("start", ui_.sb_start1->value());
                     chromosome_coordinates.insert("stop", ui_.sb_end1->value());
                    }
                    variant1.insert("chromosomeCoordinates", chromosome_coordinates);

                    //optional
                    QJsonArray gene;
                    {
                        //TODO: parse gene entries for first variant
                    }
                    variant1.insert("gene", gene);
                }
                variants.append(variant1);

                //2. variant
                QJsonObject variant2;
                {
                    //required (except hgvs)
                    QJsonObject chromosome_coordinates;
                    {
                     chromosome_coordinates.insert("alternateAllele", ui_.le_alt2->text());
                     chromosome_coordinates.insert("assembly", ui_.cb_assembly->currentText());
                     chromosome_coordinates.insert("chromosome", ui_.cb_chr2->currentText());
                     chromosome_coordinates.insert("start", ui_.sb_start2->value());
                     chromosome_coordinates.insert("stop", ui_.sb_end2->value());
                    }
                    variant2.insert("chromosomeCoordinates", chromosome_coordinates);

                    //optional
                    QJsonArray gene;
                    {
                        //TODO: parse gene entries for second variant
                    }
                    variant2.insert("gene", gene);
                }
                variants.append(variant2);
            }
            variant_set.insert("variant", variants);

        }
        clinvar_submission.insert("variantSet", variant_set);


    }
    json.insert("clinvarSubmission", QJsonArray() << clinvar_submission);


    //optional
    json.insert("submissionName", "");


    return json;
}

//QByteArray ClinvarUploadDialog::createJson()
//{
//	QByteArray output;
//	QTextStream stream(&output);

//	//create header part
////	QString lab = getSettings("lovd_lab");
////	QString user_name = getSettings("lovd_user_name");
////	QString user_email = getSettings("lovd_user_email");
////	QString user_id = getSettings("lovd_user_id");
////	QString user_auth_token = getSettings("lovd_user_auth_token");

//    // TODO:build up JSON

//	return output;
//}

void ClinvarUploadDialog::createJsonForVariant(QTextStream& stream, QString chr, QString gene, QString transcript, QLineEdit* hgvs_g, QLineEdit* hgvs_c, QLineEdit* hgvs_p, QComboBox* genotype, QComboBox* classification)
{
    //TODO: build up JSON for Variant
}

QString ClinvarUploadDialog::getSettings(QString key)
{
	QString output = Settings::string(key).trimmed();
	if (output.isEmpty())
	{
		THROW(FileParseException, "Settings INI file does not contain key '" + key + "'!");
	}
	return output;
}

QString ClinvarUploadDialog::convertClassification(QString classification)
{
	if (classification=="5")
	{
		return "Pathogenic";
	}
	if (classification=="4")
	{
		return "Probably Pathogenic";
	}
	if (classification=="3" || classification=="n/a" || classification=="")
	{
		return "Not Known";
	}
	if (classification=="2")
	{
		return "Probably Not Pathogenic";
	}
	if (classification=="1")
	{
		return "Non-pathogenic";
	}

	THROW(ProgrammingException, "Unknown classification '" + classification + "' in LovdUploadDialog::create(...) method!");
}

QString ClinvarUploadDialog::chromosomeToAccession(const Chromosome& chr)
{
	QByteArray chr_str = chr.strNormalized(false);
	if (chr_str=="1") return "NC_000001.10";
	else if (chr_str=="2") return "NC_000002.11";
	else if (chr_str=="3") return "NC_000003.11";
	else if (chr_str=="4") return "NC_000004.11";
	else if (chr_str=="5") return "NC_000005.9";
	else if (chr_str=="6") return "NC_000006.11";
	else if (chr_str=="7") return "NC_000007.13";
	else if (chr_str=="8") return "NC_000008.10";
	else if (chr_str=="9") return "NC_000009.11";
	else if (chr_str=="10") return "NC_000010.10";
	else if (chr_str=="11") return "NC_000011.9";
	else if (chr_str=="12") return "NC_000012.11";
	else if (chr_str=="13") return "NC_000013.10";
	else if (chr_str=="14") return "NC_000014.8";
	else if (chr_str=="15") return "NC_000015.9";
	else if (chr_str=="16") return "NC_000016.9";
	else if (chr_str=="17") return "NC_000017.10";
	else if (chr_str=="18") return "NC_000018.9";
	else if (chr_str=="19") return "NC_000019.9";
	else if (chr_str=="20") return "NC_000020.10";
	else if (chr_str=="21") return "NC_000021.8";
	else if (chr_str=="22") return "NC_000022.10";
	else if (chr_str=="X") return "NC_000023.10";
	else if (chr_str=="Y") return "NC_000024.9";
	else if (chr_str=="MT") return "NC_012920.1";

	THROW(ProgrammingException, "Unknown chromosome '" + chr_str + "' in LovdUploadDialog::create(...) method!");
}

QString ClinvarUploadDialog::convertGender(QString gender)
{
	if (gender=="n/a")
	{
		return "0";
	}
	if (gender=="male")
	{
		return "1";
	}
	if (gender=="female")
	{
		return "2";
	}
	THROW(ProgrammingException, "Unknown gender '" + gender + "' in LovdUploadDialog::create(...) method!");
}

QString ClinvarUploadDialog::convertGenotype(QString genotype)
{
	if (genotype=="het")
	{
		return "1";
	}
	if (genotype=="hom")
	{
		return "2";
	}

	THROW(ProgrammingException, "Unknown genotype '" + genotype + "' in LovdUploadDialog::create(...) method!")
}


//Define enum sets with allowed values for JSON output
const QStringList ClinvarUploadDialog::CLINICAL_SIGNIFICANCE_DESCRIPTION =
{
    "Pathogenic",
    "Likely pathogenic",
    "Uncertain significance",
    "Likely benign",
    "Benign",
    "affects",
    "association",
    "drug response",
    "confers sensitivity",
    "protective",
    "risk factor",
    "other",
    "not provided"
};
const QStringList ClinvarUploadDialog::MODE_OF_INHERITANCE =
{
    "Autosomal dominant inheritance",
    "Autosomal recessive inheritance",
    "Mitochondrial inheritance",
    "Somatic mutation",
    "Genetic anticipation",
    "Sporadic",
    "Sex-limited autosomal dominant",
    "X-linked recessive inheritance",
    "X-linked dominant inheritance",
    "Y-linked inheritance",
    "Other",
    "X-linked inheritance",
    "Codominant",
    "Autosomal unknown",
    "Autosomal dominant inheritance with maternal imprinting",
    "Autosomal dominant inheritance with paternal imprinting",
    "Multifactorial inheritance",
    "Unknown mechanism",
    "Oligogenic inheritance"
};
const QStringList ClinvarUploadDialog::AFFECTED_STATUS =
{
    "yes",
    "no",
    "unknown",
    "not provided",
    "not applicable"
};
const QStringList ClinvarUploadDialog::ALLELE_ORIGIN =
{
    "germline",
    "somatic",
    "de novo",
    "unknown",
    "not provided",
    "inherited",
    "maternal",
    "paternal",
    "biparental",
    "not-reported",
    "tested-inconclusive",
    "not applicable",
    "experimentally generated"
};
const QStringList ClinvarUploadDialog::COLLECTION_METHOD =
{
    "curation",
    "literature only",
    "reference population",
    "provider interpretation",
    "phenotyping only",
    "case-control",
    "clinical testing",
    "in vitro",
    "in vivo",
    "research",
    "not provided"
};
const QStringList ClinvarUploadDialog::STRUCT_VAR_METHOD_TYPE =
{
    "SNP array",
    "Oligo array",
    "Read depth",
    "Paired-end mapping",
    "One end anchored assembly",
    "Sequence alignment",
    "Optical mapping",
    "Curated,PCR"
};
const QStringList ClinvarUploadDialog::CHR =
{
    "",
    "chr1",
    "chr2",
    "chr3",
    "chr4",
    "chr5",
    "chr6",
    "chr7",
    "chr8",
    "chr9",
    "chr10",
    "chr11",
    "chr12",
    "chr13",
    "chr14",
    "chr15",
    "chr16",
    "chr17",
    "chr18",
    "chr19",
    "chr20",
    "chr21",
    "chr22",
    "chrX",
    "chrY",
    "chrM"
};
const QStringList ClinvarUploadDialog::VARIANT_TYPE =
{
    "Variation",
    "Insertion",
    "Mobile element insertion",
    "Novel sequence insertion",
    "Microsatellite",
    "Deletion",
    "single nucleotide variant",
    "Multiple nucleotide variation",
    "Indel",
    "Duplication",
    "Tandem duplication",
    "copy number loss",
    "copy number gain",
    "protein only",
    "Inversion",
    "Translocation",
    "Interchromosomal breakpoint",
    "Intrachromosomal breakpoint",
    "Complex"
};
