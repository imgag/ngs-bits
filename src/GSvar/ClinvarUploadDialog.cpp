#include "ClinvarUploadDialog.h"
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
    // set variant data
    QByteArray chr = data.variant.chr().strNormalized(false);
    ui_.cb_chr->setEnabled(false);
    ui_.cb_chr->setCurrentText(chr);
    ui_.le_start->setEnabled(false);
    ui_.le_start->setText(QString::number(data.variant.start()));
    ui_.le_end->setEnabled(false);
    ui_.le_end->setText(QString::number(data.variant.end()));
    ui_.le_ref->setEnabled(false);
    ui_.le_ref->setText(data.variant.ref());
    ui_.le_obs->setEnabled(false);
    ui_.le_obs->setText(data.variant.obs());

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
        row_idx++;
    }

    // set classification
    ui_.cb_clin_sig_desc->setEnabled(false);
    ui_.cb_clin_sig_desc->setCurrentText(convertClassification(data.report_variant_config.classification));

    // set inheritance mode (if available)
    ui_.cb_inheritance->setCurrentText(convertInheritance(data.report_variant_config.inheritance));

    // set genome assembly
    ui_.cb_assembly->setEnabled(false);
    ui_.cb_assembly->setCurrentText(buildToString(GSvarHelper::build(), true));

	// set allele origin for de novo variants
    if(data.report_variant_config.de_novo)
    {
        ui_.cb_allele_origin->setCurrentText("de novo");
    }

    // set affected status
    ui_.cb_affected_status->setCurrentText(convertAffectedStatus(data.affected_status));


    clinvar_upload_data_ = data;

    //validate input
    checkGuiData();
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

    // set date
    ui_.de_last_eval->setDate(QDate::currentDate());

    // set headers for disease info
    ui_.tw_disease_info->setColumnCount(2);
    ui_.tw_disease_info->setHorizontalHeaderItem(0, new QTableWidgetItem("type"));
    ui_.tw_disease_info->setHorizontalHeaderItem(1, new QTableWidgetItem("id"));
	//GUIHelper::resizeTableCells(ui_.tw_disease_info);
    ui_.tw_disease_info->setColumnWidth(0, 250);
    ui_.tw_disease_info->setColumnWidth(1, 150);

    // set defaults:
    ui_.cb_collection_method->setCurrentText("clinical testing");


    //connect signal and slots
    connect(ui_.upload_btn, SIGNAL(clicked(bool)), this, SLOT(upload()));
    connect(ui_.cb_chr, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
    connect(ui_.le_gene, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
    connect(ui_.phenos, SIGNAL(phenotypeSelectionChanged()), this, SLOT(checkGuiData()));
    connect(ui_.print_btn, SIGNAL(clicked(bool)), this, SLOT(printResults()));
    connect(ui_.comment_upload, SIGNAL(textChanged()), this, SLOT(updatePrintButton()));

    checkGuiData();
}

void ClinvarUploadDialog::upload()
{
	//deactivate upload button
	ui_.upload_btn->setEnabled(false);

    QJsonObject clinvar_submission = createJson();

    QStringList errors;
    if (!validateJson(clinvar_submission, errors))
    {
        QMessageBox::warning(this, "JSON validation failed", "The generated JSON contains the following errors: \n" + errors.join("\n"));
        return;
    }

    ui_.comment_upload->setText("JSON validation successful.\n");

    // read API key
    QByteArray api_key = getSettings("clinvar_api_key").toUtf8();

    QJsonObject post_request;
    QJsonArray actions;
    QJsonObject action;
    action.insert("type", "AddData");
    action.insert("targetDb", "clinvar");
    QJsonObject data;
    data.insert("content", clinvar_submission);
    action.insert("data", data);
    actions.append(action);
    post_request.insert("actions", actions);


    // perform upload
    static HttpHandler http_handler(HttpRequestHandler::INI); //static to allow caching of credentials
    try
    {
        QStringList messages;
        messages << ui_.comment_upload->toPlainText();

        //add headers
        HttpHeaders add_headers;
        add_headers.insert("Content-Type", "application/json");
        add_headers.insert("SP-API-KEY", api_key);

        //post request
		QByteArray reply = http_handler.post("https://submit.ncbi.nlm.nih.gov/api/v1/submissions/", QJsonDocument(post_request).toJson(QJsonDocument::Compact), add_headers);

        // parse response
        bool success = false;
        QString submission_id;
        QJsonObject response = QJsonDocument::fromJson(reply).object();

        //successful dry-run
        if (response.isEmpty())
        {
            messages << "MESSAGE: Dry-run successful!";
//			QFile jsonFile(clinvar_upload_data_.processed_sample + "_submission_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json");
//			jsonFile.open(QFile::WriteOnly);
//			jsonFile.write(QJsonDocument(clinvar_submission).toJson());
        }
        else if (response.contains("id"))
        {
            //successfully submitted
            messages << "MESSAGE: The submission was successful!";
            submission_id = response.value("id").toString();
            messages << "MESSAGE: Submission ID: " + submission_id;
            success = true;
        }
        else if (response.contains("message"))
        {
            //errors
            messages << "ERROR: " + response.value("message").toString();
        }

        if (success)
        {
            // add entry to the NGSD
            QStringList details;
            details << "submission_id=" + submission_id;
            //clinical significance
            details << "clinical_significance_desc=" + ui_.cb_clin_sig_desc->currentText();
            details << "clinical_significance_comment=" + VcfFile::encodeInfoValue(ui_.le_clin_sig_desc_comment->text());
            details << "last_evaluatued=" + ui_.de_last_eval->date().toString("yyyy-MM-dd");
            details << "mode_of_inheritance=" + ui_.cb_inheritance->currentText();
            //condition set
            QStringList condition;
            for (int row_idx = 0; row_idx < ui_.tw_disease_info->rowCount(); ++row_idx)
            {
                condition << ui_.tw_disease_info->item(row_idx, 0)->text() + "|" + ui_.tw_disease_info->item(row_idx, 1)->text();
            }
            details << "condition=" + condition.join(',');
			details << "variant_id=" + QString::number(clinvar_upload_data_.variant_id);
			details << "variant_rc_id=" + QString::number(clinvar_upload_data_.variant_report_config_id);
            //observed in
            details << "affected_status=" + ui_.cb_affected_status->currentText();
            details << "allele_origin=" + ui_.cb_allele_origin->currentText();
            QStringList phenotypes;
            foreach (const Phenotype& phenotype, ui_.phenos->selectedPhenotypes())
            {
                phenotypes << phenotype.accession();
            }
            details << "clinical_features=" + phenotypes.join(',');
            details << "clinical_feature_comment=" + VcfFile::encodeInfoValue(ui_.le_clin_feat_comment->text());
			details << "collection_method=" + ui_.cb_collection_method->currentText();

            details << "release_status=" + ui_.cb_release_status->currentText();
            details << "gene=" +  NGSD().genesToApproved(GeneSet::createFromStringList(ui_.le_gene->text().replace(";", ",").split(','))).toStringList().join(',');

            // log publication in NGSD
            db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.variant, "ClinVar", clinvar_upload_data_.report_variant_config.classification, details.join(";"));

            //show result
            QStringList lines;
            lines << "DATA UPLOAD TO CLINVAR SUCCESSFUL";
            lines << "";
            lines << messages.join("\n");
            lines << "";
            lines << "sample: " + clinvar_upload_data_.processed_sample;
            lines << "user: " + LoginManager::user();
            lines << "date: " + Helper::dateTime();
            lines << "";
            lines << details;

            ui_.comment_upload->setText(lines.join("\n").replace("=", ": "));

            //write report file to transfer folder
            QString gsvar_publication_folder = Settings::path("gsvar_publication_folder");
            if (gsvar_publication_folder!="")
            {
					QString file_rep = gsvar_publication_folder + "/" + clinvar_upload_data_.processed_sample + "_CLINVAR_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
                    Helper::storeTextFile(file_rep, ui_.comment_upload->toPlainText().split("\n"));
            }

        }
        else
        {
            // Upload failed
            ui_.comment_upload->setText("DATA UPLOAD ERROR:\n" + messages.join("\n"));
            ui_.upload_btn->setEnabled(true);
        }
    }
    catch(Exception e)
    {
        ui_.comment_upload->setText("DATA UPLOAD FAILED:\n" + e.message());
        ui_.upload_btn->setEnabled(true);
    }

}

bool ClinvarUploadDialog::checkGuiData()
{
	ui_.comment_upload->clear();

    //check if already published
	bool uploaded_to_clinvar = false;
	QString upload_details;
    if (clinvar_upload_data_.processed_sample !="" && clinvar_upload_data_.variant.isValid())
    {
		upload_details = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.variant);
        if (upload_details!="")
        {
			//check if uploaded to Clinvar
			foreach (const QString& line, upload_details.split('\n'))
			{
				QStringList columns = line.split(' ');
				for (int i = 0; i < (columns.size()-1); ++i)
				{
					if (columns[i] == "db:")
					{
						if (columns[i+1] == "ClinVar")
						{
							// already uploaded to ClinVar
							uploaded_to_clinvar = true;
						}
						break;
					}
				}
				// shortcut
				if (uploaded_to_clinvar) break;
			}
        }
    }

    //perform checks
	QStringList errors;
    // check chromosome
    if (ui_.cb_chr->currentText().trimmed().isEmpty())
    {
        errors << "Chromosome unset!";
    }

    // check sequences
    QRegExp re("[-]|[ACGTU]*");
    if (!re.exactMatch(ui_.le_ref->text()))
    {
        errors << "invalid reference sequence '" + ui_.le_ref->text() + "'!";
    }
    if (!re.exactMatch(ui_.le_obs->text()))
    {
        errors << "invalid observed sequence '" + ui_.le_obs->text() + "'!";
    }

    //check genes
    GeneSet gene_set = GeneSet::createFromStringList(ui_.le_gene->text().split(','));
    QStringList invalid_genes;
    foreach (const QByteArray gene, gene_set)
    {
        QByteArray approved_gene_name = NGSD().geneToApproved(gene, false);
        if(approved_gene_name.isEmpty() || (gene != approved_gene_name))
        {
            invalid_genes << gene;
        }
    }
    if (invalid_genes.size() > 0)
    {
        errors << (invalid_genes.join(", ") + " are not HGNC approved gene names!");
    }

    // check phenotypes
    if (ui_.phenos->selectedPhenotypes().count()==0)
    {
        errors << "No phenotypes selected!";
    }

    // check inheritance
    if (!MODE_OF_INHERITANCE.contains(ui_.cb_inheritance->currentText()))
    {
        errors << "No valid inheritance mode selected!";
    }

	QStringList upload_comment_text;
	if (uploaded_to_clinvar)
	{
		upload_comment_text << "<font color='red'>WARNING: This variant has already been uploaded to ClinVar! Are you sure you want to upload it again? </font><br>" + upload_details.replace("\n", "<br>");
	}


    //show error or enable upload button
    if (errors.count()>0)
    {
        ui_.upload_btn->setEnabled(false);
		upload_comment_text << "<font color='red'>Cannot upload data because:</font><br>  - " +  errors.join("<br>  - ");
		ui_.comment_upload->setText(upload_comment_text.join("<br><br>"));
        return false;
    }
    else
    {
        ui_.upload_btn->setEnabled(true);
		ui_.comment_upload->setText(upload_comment_text.join("<br>"));
        return true;
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
    //Check GUI for correct entries
    if (!checkGuiData())
    {
        QMessageBox::warning(this, "Gui validation failed!", "There are errors in you GUI input. Please fix them and try again.");
        return QJsonObject();
    }

    //build up JSON
    QJsonObject json;

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
            if (!ui_.le_clin_sig_desc_comment->text().trimmed().isEmpty())
            {
                clinical_significance.insert("comment", ui_.le_clin_sig_desc_comment->text());
            }

            //optional
            clinical_significance.insert("dateLastEvaluated", ui_.de_last_eval->date().toString("yyyy-MM-dd"));

            //optional
            if (!ui_.cb_inheritance->currentText().trimmed().isEmpty())
            {
                clinical_significance.insert("modeOfInheritance", ui_.cb_inheritance->currentText());
            }
        }
        clinvar_submission.insert("clinicalSignificance", clinical_significance);

        //optional
        // clinvar_submission.insert("clinvarAccession", "");

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
						//extract ID
						QString omim_id = ui_.tw_disease_info->item(row_idx, 1)->text();
						if (!omim_id.startsWith("#")) THROW(ArgumentException, "Invalid OMIM id '" + ui_.tw_disease_info->item(row_idx, 1)->text() + "'!");
						omim_id = omim_id.split('#').at(1);
						bool ok;
						omim_id.toInt(&ok);
						if (!ok) THROW(ArgumentException, "Invalid OMIM id '" + ui_.tw_disease_info->item(row_idx, 1)->text() + "'!");
						disease_info.insert("id", omim_id);
                        condition.append(disease_info);
                    }
                    else if (ui_.tw_disease_info->item(row_idx, 0)->text() == "Orpha number")
                    {
                        QJsonObject disease_info;
                        disease_info.insert("db", "Orphanet");
						//extract ID
						QString orphanet_id = ui_.tw_disease_info->item(row_idx, 1)->text();
						if (!orphanet_id.startsWith("ORPHA:")) THROW(ArgumentException, "Invalid Orphanet id '" + ui_.tw_disease_info->item(row_idx, 1)->text() + "'!");
						orphanet_id = orphanet_id.split(':').at(1);
						bool ok;
						orphanet_id.toInt(&ok);
						if (!ok) THROW(ArgumentException, "Invalid Orphanet id '" + ui_.tw_disease_info->item(row_idx, 1)->text() + "'!");
						disease_info.insert("id", orphanet_id);
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
        clinvar_submission.insert("localID", QString::number(clinvar_upload_data_.variant_id));

        //optional
        clinvar_submission.insert("localKey", QString::number(clinvar_upload_data_.variant_report_config_id));

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
                    QJsonObject feature;
                    feature.insert("db", "HP");
                    feature.insert("id", QString(phenotype.accession()));
                    clinical_features.append(feature);

                }
            }
            observed_in.insert("clinicalFeatures", clinical_features);

            //optional
            if (!ui_.le_clin_feat_comment->text().trimmed().isEmpty())
            {
                observed_in.insert("clinicalFeaturesComment", ui_.le_clin_feat_comment->text());
            }

            //required
            observed_in.insert("collectionMethod", ui_.cb_collection_method->currentText());

            //optional
            //observed_in.insert("numberOfIndividuals", ui_.sb_n_individuals->value());

            //optional
            //observed_in.insert("structVarMethodType", ui_.cb_method_type->currentText());

        }
        clinvar_submission.insert("observedIn", QJsonArray() << observed_in);

        //required
		clinvar_submission.insert("recordStatus", "novel");

        //required
        clinvar_submission.insert("releaseStatus", ui_.cb_release_status->currentText());

        //required
        QJsonObject variant_set;
        {
            QJsonArray variants;
            {
                //variant
                QJsonObject variant;
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
                    variant.insert("chromosomeCoordinates", chromosome_coordinates);

					//optional (but required for deletions and insertions)
					QString variant_type;
					if (ui_.le_ref->text().contains("-") || ui_.le_ref->text().trimmed().isEmpty()) variant_type = "Insertion";
					if (ui_.le_obs->text().contains("-") || ui_.le_obs->text().trimmed().isEmpty()) variant_type = "Deletion";
					if (!variant_type.isEmpty()) variant.insert("variantType", variant_type);

                    //optional
                    if (!ui_.le_gene->text().trimmed().isEmpty())
                    {
                        QJsonArray genes;
                        {
                            GeneSet gene_set = NGSD().genesToApproved(GeneSet::createFromStringList(ui_.le_gene->text().replace(";", ",").split(',')));
                            foreach (const QByteArray& gene_name, gene_set)
                            {
                                QJsonObject gene;
                                gene.insert("symbol", QString(gene_name));
                                genes.append(gene);
                            }
                        }
                        variant.insert("gene", genes);
                    }

                }
                variants.append(variant);
            }
            variant_set.insert("variant", variants);

        }
        clinvar_submission.insert("variantSet", variant_set);


    }
    json.insert("clinvarSubmission", QJsonArray() << clinvar_submission);

    //optional
    //json.insert("submissionName", "");

    return json;
}

bool ClinvarUploadDialog::validateJson(const QJsonObject& json, QStringList& errors)
{
    bool is_valid = true;
    // check clinvarSubmission
    if (!json.contains("clinvarSubmission"))
    {
        errors << "Required JSON object 'clinvarSubmission' missing!";
        return false;
    }

    QJsonArray clinvar_submission_array = json.value("clinvarSubmission").toArray();
    if (clinvar_submission_array.size() < 1)
    {
        errors << "No entry in 'clinvarSubmission'!";
        return false;
    }
    else if (clinvar_submission_array.size() > 1)
    {
        errors << "Multiple entries in 'clinvarSubmission'!";
        return false;
    }
    QJsonObject clinvar_submission = clinvar_submission_array[0].toObject();

    // check required entries
    if (clinvar_submission.contains("recordStatus"))
    {
        QStringList record_status = QStringList() <<  "novel" << "update";
        if (!record_status.contains(clinvar_submission.value("recordStatus").toString()))
        {
            errors << "Invalid entry '" + clinvar_submission.value("recordStatus").toString() + "' in 'recordStatus'!";
            is_valid = false;
        }
    }
    else
    {
        errors << "Required string 'recordStatus' in 'clinvarSubmission' missing!";
        is_valid = false;
    }

    if (clinvar_submission.contains("releaseStatus"))
    {
        QStringList release_status = QStringList() <<  "public" << "hold until published";
        if (!release_status.contains(clinvar_submission.value("releaseStatus").toString()))
        {
            errors << "Invalid entry '" + clinvar_submission.value("releaseStatus").toString() + "' in 'releaseStatus'!";
            is_valid = false;
        }
    }
    else
    {
        errors << "Required string 'releaseStatus' in 'clinvarSubmission' missing!";
        is_valid = false;
    }

    if (clinvar_submission.contains("clinicalSignificance"))
    {
        //parse clinicalSignificance
        QJsonObject clinical_significance = clinvar_submission.value("clinicalSignificance").toObject();
        if (clinical_significance.contains("clinicalSignificanceDescription"))
        {
            if (!CLINICAL_SIGNIFICANCE_DESCRIPTION.contains(clinical_significance.value("clinicalSignificanceDescription").toString()))
            {
                errors << "Invalid entry '" + clinical_significance.value("clinicalSignificanceDescription").toString() + "' in 'clinicalSignificanceDescription'!";
                is_valid = false;
            }
        }
        else
        {
            errors << "Required string 'clinicalSignificanceDescription' in 'clinicalSignificance' missing!";
            is_valid = false;
        }

    }
    else
    {
        errors << "Required string 'clinicalSignificance' in 'clinvarSubmission' missing!";
        is_valid = false;
    }

    if (clinvar_submission.contains("observedIn"))
    {
        // allow only one entry in observedIn array
        QJsonArray observed_in_array = clinvar_submission.value("observedIn").toArray();
        if (observed_in_array.size() < 1)
        {
            errors << "No entry in 'observedIn'!";
            return false;
        }
        else if (observed_in_array.size() > 1)
        {
            errors << "Multiple entries in 'observedIn'!";
            return false;
        }

        //parse observedIn
        QJsonObject observed_in = observed_in_array[0].toObject();
        if (observed_in.contains("alleleOrigin"))
        {
            if (!ALLELE_ORIGIN.contains(observed_in.value("alleleOrigin").toString()))
            {
                errors << "Invalid entry '" + observed_in.value("alleleOrigin").toString() + "' in 'observedIn'!";
                is_valid = false;
            }
        }
        else
        {
            errors << "Required string 'alleleOrigin' in 'observedIn' missing!";
            is_valid = false;
        }
        if (observed_in.contains("affectedStatus"))
        {
            if (!AFFECTED_STATUS.contains(observed_in.value("affectedStatus").toString()))
            {
                errors << "Invalid entry '" + observed_in.value("affectedStatus").toString() + "' in 'observedIn'!";
                is_valid = false;
            }
        }
        else
        {
            errors << "Required string 'affectedStatus' in 'observedIn' missing!";
            is_valid = false;
        }
        if (observed_in.contains("collectionMethod"))
        {
            if (!COLLECTION_METHOD.contains(observed_in.value("collectionMethod").toString()))
            {
                errors << "Invalid entry '" + observed_in.value("collectionMethod").toString() + "' in 'observedIn'!";
                is_valid = false;
            }
        }
        else
        {
            errors << "Required string 'collectionMethod' in 'observedIn' missing!";
            is_valid = false;
        }

    }
    else
    {
        errors << "Required JSON object 'observedIn' in 'clinvarSubmission' missing!";
        is_valid = false;
    }

    if (clinvar_submission.contains("variantSet"))
    {
        //parse variantSet
        QJsonObject variant_set = clinvar_submission.value("variantSet").toObject();
        if (variant_set.contains("variant"))
        {
            QJsonArray variant_array = variant_set.value("variant").toArray();
            if (variant_array.size() > 0)
            {
                foreach (const QJsonValue& variant, variant_array)
                {
                    if (variant.toObject().contains("chromosomeCoordinates"))
                    {
                        QJsonObject chromosome_coordinates = variant.toObject().value("chromosomeCoordinates").toObject();
                        if (chromosome_coordinates.contains("chromosome"))
                        {
                            if (!CHR.contains(chromosome_coordinates.value("chromosome").toString()))
                            {
                                errors << "Invalid entry '" + chromosome_coordinates.value("chromosome").toString() + "' in 'chromosome'!";
                                is_valid = false;
                            }
                        }
                        else
                        {
                            errors << "Required string 'chromosome' in 'chromosomeCoordinates' missing!";
                            is_valid = false;
                        }

                        if (chromosome_coordinates.contains("start"))
                        {
                            bool* ok = new bool();
                            chromosome_coordinates.value("start").toString().toInt(ok);
                            if (!ok)
                            {
                                errors << "Invalid entry '" + chromosome_coordinates.value("start").toString() + "' in 'start'!";
                                is_valid = false;
                            }
                        }
                        else
                        {
                            errors << "Required number 'start' in 'chromosomeCoordinates' missing!";
                            is_valid = false;
                        }

                        if (chromosome_coordinates.contains("stop"))
                        {
                            bool* ok = new bool();
                            chromosome_coordinates.value("stop").toString().toInt(ok);
                            if (!ok)
                            {
                                errors << "Invalid entry '" + chromosome_coordinates.value("stop").toString() + "' in 'stop'!";
                                is_valid = false;
                            }
                        }
                        else
                        {
                            errors << "Required number 'stop' in 'chromosomeCoordinates' missing!";
                            is_valid = false;
                        }

                        if (chromosome_coordinates.contains("referenceAllele"))
                        {
                            QString ref = chromosome_coordinates.value("referenceAllele").toString().trimmed();
                            QRegExp re("[-]|[ACGTUacgtu]+");
                            if (!re.exactMatch(ref))
                            {
                                errors << "Invalid entry '" + ref + "' in 'referenceAllele'!";
                                is_valid = false;
                            }
                        }
                        else
                        {
                            errors << "Required string 'referenceAllele' in 'chromosomeCoordinates' missing!";
                            is_valid = false;
                        }

                        if (chromosome_coordinates.contains("alternateAllele"))
                        {
                            QString alt = chromosome_coordinates.value("alternateAllele").toString().trimmed();
                            QRegExp re("[-]|[ACGTUacgtu]+");
                            if (alt != "-" && !alt.isEmpty() && !re.exactMatch(alt))
                            {
                                errors << "Invalid entry '" + alt + "' in 'alternateAllele'!";
                                is_valid = false;
                            }
                        }
                        else
                        {
                            errors << "Required string 'alternateAllele' in 'chromosomeCoordinates' missing!";
                            is_valid = false;
                        }

                    }
                    else
                    {
                        errors << "Required JSON object 'chromosomeCoordinates' in 'variant' missing!";
                        is_valid = false;
                    }

					if (variant.toObject().contains("variantType"))
					{
						QString variant_type = variant.toObject().value("chromosomeCoordinates").toString();
						if (!VARIANT_TYPE.contains(variant_type))
						{
							errors << "Invalid variantType '" + variant_type + "'!";
							is_valid = false;
						}
					}
                }
            }
            else
            {
				errors << "JSON array 'variant' in 'variantSet' has to have at least one entry!";
                is_valid = false;
            }
        }
        else
        {
            errors << "Required JSON array 'variant' in 'variantSet' missing!";
            is_valid = false;
        }

    }
    else
    {
        errors << "Required string 'variantSet' in 'clinvarSubmission' missing!";
        is_valid = false;
    }

    if (clinvar_submission.contains("conditionSet"))
    {
        //parse variantSet
        QJsonObject condition_set = clinvar_submission.value("conditionSet").toObject();
        if (condition_set.contains("condition"))
        {
            QJsonArray condition_array = condition_set.value("condition").toArray();
            if (condition_array.size() > 0)
            {
                foreach (const QJsonValue& condition, condition_array)
                {
                    if (condition.toObject().contains("db"))
                    {
                        QString db = condition.toObject().value("db").toString();
                        if (db == "OMIM")
                        {
                            if (condition.toObject().contains("id"))
                            {
                                QString id = condition.toObject().value("id").toString();
								bool ok;
								id.toInt(&ok);
								if (!ok)
                                {
                                    errors << "Invalid entry '" + id + "' in 'id'!";
                                    is_valid = false;
                                }
                            }
                            else
                            {
                                errors << "Required string 'id' in 'condition' missing!";
                                is_valid = false;
                            }
                        }
                        else if (db == "Orphanet")
                        {
                            if (condition.toObject().contains("id"))
                            {
                                QString id = condition.toObject().value("id").toString();
								bool ok;
								id.toInt(&ok);
								if (!ok)
								{
                                    errors << "Invalid entry '" + id + "' in 'id'!";
                                    is_valid = false;
                                }
                            }
                            else
                            {
                                errors << "Required string 'id' in 'condition' missing!";
                                is_valid = false;
                            }
                        }
                        else
                        {
                            errors << "Invalid entry '" + db + "' in 'db'!";
                            is_valid = false;
                        }
                    }
                    else
                    {
                        errors << "Required string 'db' in 'condition' missing!";
                        is_valid = false;
                    }
                }
            }
            else
            {
                errors << "JSON array 'condition' in 'conditionSet' has to have at least one entry!";
                is_valid = false;
            }
        }
        else
        {
            errors << "Required JSON array 'variant' in 'variantSet' missing!";
            is_valid = false;
        }

    }
    else
    {
        errors << "Required string 'conditionSet' in 'clinvarSubmission' missing!";
        is_valid = false;
    }

    return is_valid;
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
	"MT"
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
