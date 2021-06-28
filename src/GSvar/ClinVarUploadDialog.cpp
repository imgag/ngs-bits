#include "ClinVarUploadDialog.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "LoginManager.h"
#include "GSvarHelper.h"
#include "GUIHelper.h"
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
    if (!LoginManager::active())
    {
        QMessageBox::warning(this, "No NGSD connection", "ClinVar Upload requires access to the NGSD");
        return;
    }
    ui_.setupUi(this);

    initGui();
}

void ClinvarUploadDialog::setData(ClinvarUploadData data)
{
    // set ids
    ui_.le_local_id->setEnabled(false);
    ui_.le_local_id->setText(QString::number(data.variant_id));
    ui_.le_local_key->setEnabled(false);
    ui_.le_local_key->setText(QString::number(data.variant_report_config_id));

    // set variant data
    variant1_ = data.variant;
    QByteArray chr = data.variant.chr().strNormalized(false);
    if (chr=="MT") chr = "M";
    ui_.cb_chr->setEnabled(false);
    ui_.cb_chr->setCurrentText(chr);
    ui_.le_start->setEnabled(false);
    ui_.le_start->setText(QString::number(variant1_.start()));
    ui_.le_end->setEnabled(false);
    ui_.le_end->setText(QString::number(variant1_.end()));
    ui_.le_ref->setEnabled(false);
    ui_.le_ref->setText(variant1_.ref());
    ui_.le_obs->setEnabled(false);
    ui_.le_obs->setText(variant1_.obs());
    ui_.cb_var_type->setEnabled(false);
    ui_.cb_var_type->setCurrentText((variant1_.isSNV())?"Variation":"Indel");

    // set genes
    ui_.le_gene->setText(data.genes.join(","));

    // set phenotypes
    ui_.phenos->setPhenotypes(data.phenos);

    // set disease info
    ui_.tw_disease_info->setRowCount(data.disease_info.length());
    int row_idx = 0;
    foreach (const SampleDiseaseInfo& disease, data.disease_info)
    {
        ui_.tw_disease_info->setItem(row_idx, 0, new QTableWidgetItem(disease.type));
        ui_.tw_disease_info->setItem(row_idx, 1, new QTableWidgetItem(disease.disease_info));
    }

    // set classification
    ui_.cb_clin_sig_desc->setEnabled(false);
    ui_.cb_clin_sig_desc->setCurrentText(convertClassification(data.report_variant_config.classification));

    // set inheritance mode (if available)
    ui_.cb_inheritance->setCurrentText(convertInheritance(data.report_variant_config.inheritance));

    // set genome assembly
    ui_.cb_assembly->setEnabled(false);
    ui_.cb_assembly->setCurrentText(buildToString(GSvarHelper::build(), true));

    // set allele origin for de novo varaints
    if(data.report_variant_config.de_novo)
    {
        ui_.cb_allele_origin->setCurrentText("de novo");
    }

    // set affected status
    ui_.cb_affected_status->setCurrentText(convertAffectedStatus(data.affected_status));


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
    ui_.cb_assembly->addItems(ASSEMBLY);
    ui_.cb_chr->addItems(CHR);
    ui_.cb_var_type->addItems(VARIANT_TYPE);

    // set date
    ui_.de_last_eval->setDate(QDate::currentDate());

    // set headers for disease info
    ui_.tw_disease_info->setColumnCount(2);
    ui_.tw_disease_info->setHorizontalHeaderItem(0, new QTableWidgetItem("type"));
    ui_.tw_disease_info->setHorizontalHeaderItem(1, new QTableWidgetItem("id"));
//    GUIHelper::resizeTableCells(ui_.tw_disease_info);
    ui_.tw_disease_info->setColumnWidth(0, 250);
    ui_.tw_disease_info->setColumnWidth(1, 150);

    // set defaults:
    ui_.cb_collection_method->setCurrentText("clinical testing");







    //connect signal and slots
    connect(ui_.upload_btn, SIGNAL(clicked(bool)), this, SLOT(upload()));

    connect(ui_.cb_chr, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
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
	//TODO: check if already published
//	if (ui_.processed_sample->text()!="" && variant1_.isValid())
//	{
//		QString upload_details = db_.getVariantPublication(ui_.processed_sample->text(), variant1_);
//		if (upload_details!="")
//		{
//			ui_.upload_btn->setEnabled(false);
//			ui_.comment_upload->setText("<font color='red'>ERROR: variant already uploaded!</font><br>" + upload_details);
//			return;
//		}
//	}

	//perform checks
	QStringList errors;
        if (ui_.cb_chr->currentText().trimmed().isEmpty())
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
            // add hardcoded citation
            QJsonObject citation;
            citation.insert("db", "PubMed");
            citation.insert("id", "25741868");
            assertion_criteria.insert("citation", citation);
            assertion_criteria.insert("method", "ACMG Guidelines, 2015");
        }
        clinvar_submission.insert("assertionCriteria", assertion_criteria);

        //required
        QJsonObject clinical_significance;
        {
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
                for (int row_idx = 0; row_idx < ui_.tw_disease_info->rowCount(); ++row_idx)
                {
                    if (ui_.tw_disease_info->item(row_idx, 0)->text() == "OMIM disease/phenotype identifier")
                    {
                        QJsonObject disease_info;
                        disease_info.insert("db", "OMIM");
                        disease_info.insert("id", ui_.tw_disease_info->item(row_idx, 1)->text());
                        condition.append(disease_info);
                    }
                    else if (ui_.tw_disease_info->item(row_idx, 0)->text() == "Orpha number")
                    {
                        QJsonObject disease_info;
                        disease_info.insert("db", "Orphanet");
                        disease_info.insert("id", ui_.tw_disease_info->item(row_idx, 1)->text());
                        condition.append(disease_info);
                    }
                    else
                    {
                        THROW(ArgumentException, "Invalid disease info '" + ui_.tw_disease_info->item(row_idx, 0)->text() + "' in disease info table!");
                    }

                }
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
                foreach (const Phenotype& phenotype, ui_.phenos->selectedPhenotypes())
                {
                    qDebug() << phenotype.accession().split(':')[1].toInt();
                    QJsonObject feature;
                    feature.insert("db", "HP");
                    feature.insert("id", QString::number(phenotype.accession().split(':')[1].toInt()));
                    clinical_features.append(feature);

                }
            }
            observed_in.insert("clinicalFeatures", clinical_features);

            //optional
            observed_in.insert("clinicalFeaturesComment", ui_.le_clin_feat_comment->text());

            //required
            observed_in.insert("collectionMethod", ui_.cb_collection_method->currentText());

            //optional
//            observed_in.insert("numberOfIndividuals", ui_.sb_n_individuals->value());

            //optional
//            observed_in.insert("structVarMethodType", ui_.cb_method_type->currentText());

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
                        chromosome_coordinates.insert("alternateAllele", ui_.le_obs->text());
                        chromosome_coordinates.insert("assembly", ui_.cb_assembly->currentText());
                        chromosome_coordinates.insert("chromosome", ui_.cb_chr->currentText());
                        chromosome_coordinates.insert("referenceAllele", ui_.le_ref->text());
                        chromosome_coordinates.insert("start", Helper::toInt(ui_.le_start->text()));
                        chromosome_coordinates.insert("stop", Helper::toInt(ui_.le_end->text()));
                    }
                    variant1.insert("chromosomeCoordinates", chromosome_coordinates);

                    //optional
                    QJsonArray genes;
                    {
                        GeneSet gene_set = NGSD().genesToApproved(GeneSet::createFromStringList(ui_.le_gene->text().split(',')));
                        foreach (const QByteArray& gene_name, gene_set)
                        {
                            QJsonObject gene;
                            gene.insert("symbol", QString(gene_name));
                            genes.append(gene);
                        }
                    }
                    variant1.insert("gene", genes);
                }
                variants.append(variant1);
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
                return "Likely pathogenic";
	}
	if (classification=="3" || classification=="n/a" || classification=="")
	{
                return "Uncertain significance";
	}
	if (classification=="2")
	{
                return "Likely benign";
	}
	if (classification=="1")
	{
                return "Benign";
	}
        else if (classification=="M")
        {
                return "risk factor";
        }

        THROW(ProgrammingException, "Unknown classification '" + classification + "' in ClinvarUploadDialog::convertClassification(...) method!");
}

QString ClinvarUploadDialog::convertInheritance(QString inheritance)
{
    if (inheritance == "AR")
    {
        return "Autosomal recessive inheritance";
    }
    if (inheritance == "AD")
    {
        return "Autosomal dominant inheritance";
    }
    if (inheritance == "XLR")
    {
        return "X-linked recessive inheritance";
    }
    if (inheritance == "XLD")
    {
        return "X-linked dominant inheritance";
    }
    if (inheritance == "MT")
    {
        return "Mitochondrial inheritance";
    }
    if (inheritance == "n/a")
    {
        return "";
    }
    THROW(ProgrammingException, "Unknown inheritance mode '" + inheritance + "' in ClinvarUploadDialog::convertInheritance(...) method!");
}

QString ClinvarUploadDialog::convertAffectedStatus(QString affected_status)
{
    if (affected_status == "n/a")
    {
        return "not provided";
    }
    if (affected_status == "Affected")
    {
        return "yes";
    }
    if (affected_status == "Unaffected")
    {
        return "no";
    }
    if (affected_status == "Unclear")
    {
        return "unknown";
    }
    THROW(ProgrammingException, "Unknown affected status '" + affected_status + "' in ClinvarUploadDialog::convertAffectedStatus(...) method!");
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
    "",
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
    "",
    "SNP array",
    "Oligo array",
    "Read depth",
    "Paired-end mapping",
    "One end anchored assembly",
    "Sequence alignment",
    "Optical mapping",
    "Curated,PCR"
};
const QStringList ClinvarUploadDialog::ASSEMBLY =
{
    "GRCh38",
    "hg38",
    "GRCh37",
    "hg19",
    "NCBI36",
    "hg18"
};
const QStringList ClinvarUploadDialog::CHR =
{
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "17",
    "18",
    "19",
    "20",
    "21",
    "22",
    "X",
    "Y",
    "M"
};
const QStringList ClinvarUploadDialog::VARIANT_TYPE =
{
    "",
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
