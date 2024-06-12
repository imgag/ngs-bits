#include "ClinvarUploadDialog.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "LoginManager.h"
#include "GSvarHelper.h"
#include "GUIHelper.h"
#include "ReportVariantSelectionDialog.h"
#include "GlobalServiceProvider.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPrinter>
#include <QPrintDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>


const bool test_run = false;
const QString api_url = (test_run)? "https://submit.ncbi.nlm.nih.gov/apitest/v1/submissions" : "https://submit.ncbi.nlm.nih.gov/api/v1/submissions/";

ClinvarUploadDialog::ClinvarUploadDialog(QWidget *parent)
    : QDialog(parent)
    , ui_()
{
	if (!LoginManager::active())
	{
		INFO(DatabaseException, "ClinVar Upload requires logging in into NGSD!");
	}

	LoginManager::checkRoleNotIn(QStringList{"user_restricted"});

    ui_.setupUi(this);
	connect(ui_.tw_disease_info, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(diseaseContextMenu(QPoint)));

    initGui();
}

void ClinvarUploadDialog::setData(ClinvarUploadData data)
{
	// store given data
	clinvar_upload_data_ = data;

	// set first variant data
	if (clinvar_upload_data_.variant_type1 == VariantType::SNVS_INDELS)
	{
		//set stacked widget to SNV page
		ui_.sw_var1->setCurrentIndex(0);

		// fill fields
		ui_.cb_chr_snv1->setEnabled(true);
		ui_.cb_chr_snv1->setCurrentText(clinvar_upload_data_.snv1.chr().strNormalized(false));
		ui_.le_start_snv1->setEnabled(false);
		ui_.le_end_snv1->setEnabled(false);
		ui_.le_ref_snv1->setEnabled(false);
		ui_.le_obs_snv1->setEnabled(false);

		if (clinvar_upload_data_.snv1.isSNV())
		{
			ui_.le_start_snv1->setText(QString::number(clinvar_upload_data_.snv1.start()));
			ui_.le_end_snv1->setText(QString::number(clinvar_upload_data_.snv1.end()));
			ui_.le_ref_snv1->setText(data.snv1.ref());
			ui_.le_obs_snv1->setText(data.snv1.obs());
		}
		else
		{
			//convert indels to VCF format
			static FastaFileIndex genome_index(Settings::string("reference_genome"));
			VcfLine vcf_variant = clinvar_upload_data_.snv1.toVCF(genome_index);

			ui_.le_start_snv1->setText(QString::number(vcf_variant.start()));
			ui_.le_end_snv1->setText(QString::number(vcf_variant.end()));
			ui_.le_ref_snv1->setText(vcf_variant.ref());
			ui_.le_obs_snv1->setText(vcf_variant.altString());
		}

		// set genes
		ui_.le_genes_snv1->setText(data.genes.join(","));
	}
	else if(clinvar_upload_data_.variant_type1 == VariantType::CNVS)
	{
		//set stacked widget to SNV page
		ui_.sw_var1->setCurrentIndex(1);

		// fill fields
		ui_.cb_chr_cnv1->setEnabled(false);
		ui_.cb_chr_cnv1->setCurrentText(clinvar_upload_data_.cnv1.chr().strNormalized(false));
		ui_.le_start_cnv1->setEnabled(false);
		ui_.le_start_cnv1->setText(QString::number(clinvar_upload_data_.cnv1.start()));
		ui_.le_end_cnv1->setEnabled(false);
		ui_.le_end_cnv1->setText(QString::number(clinvar_upload_data_.cnv1.end()));

		ui_.le_cn_cnv1->setEnabled(false);
		ui_.le_cn_cnv1->setText(QString::number(clinvar_upload_data_.cn1));
		if(data.ref_cn1 < 0)
		{
			ui_.le_rcn_cnv1->setEnabled(true);
			ui_.le_rcn_cnv1->setText("");
		}
		else
		{
			ui_.le_rcn_cnv1->setEnabled(false);
			ui_.le_rcn_cnv1->setText(QString::number(data.ref_cn1));
		}

		// set genes
		ui_.le_genes_cnv1->setText(data.genes.join(","));
	}
	else if(clinvar_upload_data_.variant_type1 == VariantType::SVS)
	{
		//set stacked widget to SNV page
		ui_.sw_var1->setCurrentIndex(2);

		// fill fields
		ui_.cb_type_sv1->setEnabled(false);
		ui_.cb_type_sv1->setCurrentText(BedpeFile::typeToString(clinvar_upload_data_.sv1.type()));
		ui_.cb_chr1_sv1->setEnabled(false);
		ui_.cb_chr1_sv1->setCurrentText(clinvar_upload_data_.sv1.chr1().strNormalized(false));
		ui_.le_start1_sv1->setEnabled(false);
		ui_.le_start1_sv1->setText(QString::number(clinvar_upload_data_.sv1.start1()));
		ui_.le_end1_sv1->setEnabled(false);
		ui_.le_end1_sv1->setText(QString::number(clinvar_upload_data_.sv1.end1()));
		ui_.cb_chr2_sv1->setEnabled(false);
		ui_.cb_chr2_sv1->setCurrentText(clinvar_upload_data_.sv1.chr2().strNormalized(false));
		ui_.le_start2_sv1->setEnabled(false);
		ui_.le_start2_sv1->setText(QString::number(clinvar_upload_data_.sv1.start2()));
		ui_.le_end2_sv1->setEnabled(false);
		ui_.le_end2_sv1->setText(QString::number(clinvar_upload_data_.sv1.end2()));

		// set genes
		ui_.le_genes_sv1->setText(clinvar_upload_data_.genes.join(","));
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type provided!");
	}
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		//Compound heterozygote variant

		//set values for second variant
		if (clinvar_upload_data_.variant_type2 == VariantType::SNVS_INDELS)
		{
			//set stacked widget to SNV page
			ui_.sw_var2->setCurrentIndex(0);

			// fill fields
			ui_.cb_chr_snv2->setEnabled(false);
			ui_.cb_chr_snv2->setCurrentText(clinvar_upload_data_.snv2.chr().strNormalized(false));
			ui_.le_start_snv2->setEnabled(false);
			ui_.le_end_snv2->setEnabled(false);
			ui_.le_ref_snv2->setEnabled(false);
			ui_.le_obs_snv2->setEnabled(false);

			if (clinvar_upload_data_.snv2.isSNV())
			{
				ui_.le_start_snv2->setText(QString::number(clinvar_upload_data_.snv2.start()));
				ui_.le_end_snv2->setText(QString::number(clinvar_upload_data_.snv2.end()));
				ui_.le_ref_snv2->setText(clinvar_upload_data_.snv2.ref());
				ui_.le_obs_snv2->setText(clinvar_upload_data_.snv2.obs());
			}
			else
			{
				//convert indels to VCF format
				static FastaFileIndex genome_index(Settings::string("reference_genome"));
				VcfLine vcf_variant = clinvar_upload_data_.snv2.toVCF(genome_index);

				ui_.le_start_snv2->setText(QString::number(vcf_variant.start()));
				ui_.le_end_snv2->setText(QString::number(vcf_variant.end()));
				ui_.le_ref_snv2->setText(vcf_variant.ref());
				ui_.le_obs_snv2->setText(vcf_variant.altString());
			}

		}
		else if(clinvar_upload_data_.variant_type2 == VariantType::CNVS)
		{
			//set stacked widget to SNV page
			ui_.sw_var2->setCurrentIndex(1);

			// fill fields
			ui_.cb_chr_cnv2->setEnabled(false);
			ui_.cb_chr_cnv2->setCurrentText(clinvar_upload_data_.cnv2.chr().strNormalized(false));
			ui_.le_start_cnv2->setEnabled(false);
			ui_.le_start_cnv2->setText(QString::number(clinvar_upload_data_.cnv2.start()));
			ui_.le_end_cnv2->setEnabled(false);
			ui_.le_end_cnv2->setText(QString::number(clinvar_upload_data_.cnv2.end()));

			ui_.le_cn_cnv2->setEnabled(false);
			ui_.le_cn_cnv2->setText(QString::number(clinvar_upload_data_.cn2));
			if(clinvar_upload_data_.ref_cn2 < 0)
			{
				ui_.le_rcn_cnv2->setEnabled(true);
				ui_.le_rcn_cnv2->setText("");
			}
			else
			{
				ui_.le_rcn_cnv2->setEnabled(false);
				ui_.le_rcn_cnv2->setText(QString::number(clinvar_upload_data_.ref_cn2));
			}


		}
		else if(data.variant_type2 == VariantType::SVS)
		{
			//set stacked widget to SNV page
			ui_.sw_var2->setCurrentIndex(2);

			// fill fields
			ui_.cb_type_sv2->setEnabled(false);
			ui_.cb_type_sv2->setCurrentText(BedpeFile::typeToString(clinvar_upload_data_.sv2.type()));
			ui_.cb_chr1_sv2->setEnabled(false);
			ui_.cb_chr1_sv2->setCurrentText(clinvar_upload_data_.sv2.chr1().strNormalized(false));
			ui_.le_start1_sv2->setEnabled(false);
			ui_.le_start1_sv2->setText(QString::number(clinvar_upload_data_.sv2.start1()));
			ui_.le_end1_sv2->setEnabled(false);
			ui_.le_end1_sv2->setText(QString::number(clinvar_upload_data_.sv2.end1()));
			ui_.cb_chr2_sv2->setEnabled(false);
			ui_.cb_chr2_sv2->setCurrentText(clinvar_upload_data_.sv2.chr2().strNormalized(false));
			ui_.le_start2_sv2->setEnabled(false);
			ui_.le_start2_sv2->setText(QString::number(clinvar_upload_data_.sv2.start2()));
			ui_.le_end2_sv2->setEnabled(false);
			ui_.le_end2_sv2->setText(QString::number(clinvar_upload_data_.sv2.end2()));

		}
		else
		{
			THROW(ArgumentException, "Invalid variant 2 type provided!");
		}

	}

    // set phenotypes
	ui_.phenos->setPhenotypes(clinvar_upload_data_.phenos);

    // set disease info
	ui_.tw_disease_info->setRowCount(clinvar_upload_data_.disease_info.length());
    int row_idx = 0;
	foreach (const SampleDiseaseInfo& disease, clinvar_upload_data_.disease_info)
    {
        ui_.tw_disease_info->setItem(row_idx, 0, new QTableWidgetItem(disease.type));
        ui_.tw_disease_info->setItem(row_idx, 1, new QTableWidgetItem(disease.disease_info));
        row_idx++;
    }

    // set classification
	ui_.cb_clin_sig_desc->setEnabled(data.submission_type==ClinvarSubmissionType::CompoundHeterozygous);
	ui_.cb_clin_sig_desc->setCurrentText(convertClassification(data.report_variant_config1.classification, false));

    // set inheritance mode (if available)
	ui_.cb_inheritance->setCurrentText(convertInheritance(data.report_variant_config1.inheritance));

	// set allele origin for de novo variants
	if(clinvar_upload_data_.report_variant_config1.de_novo)
    {
        ui_.cb_allele_origin->setCurrentText("de novo");
    }

    // set affected status
	ui_.cb_affected_status->setCurrentText(convertAffectedStatus(clinvar_upload_data_.affected_status));

	// check for reupload
	if (clinvar_upload_data_.variant_publication_id > 0)
	{
		// reupload
		if (clinvar_upload_data_.stable_id.trimmed().isEmpty())
		{
			// reupload of failed submission
			setWindowTitle(windowTitle() + " (Reupload of failed submission by " + db_.userName(clinvar_upload_data_.user_id) + ")");
		}
		else
		{
			// modification of successful submission
			setWindowTitle(windowTitle() + " (Modification of successfull submission (" + clinvar_upload_data_.stable_id + ") by " + db_.userName(clinvar_upload_data_.user_id) + ")");
		}
	}

	// set upload type
	manual_upload_ = false;

	// (de-)activate button to add comp-het variant
	updateGUI();

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
	ui_.cb_chr_snv1->addItems(CHR);
	ui_.cb_chr_snv2->addItems(CHR);
	ui_.cb_chr_cnv1->addItems(CHR);
	ui_.cb_chr_cnv2->addItems(CHR);
	ui_.cb_chr1_sv1->addItems(CHR);
	ui_.cb_chr2_sv1->addItems(CHR);
	ui_.cb_chr1_sv2->addItems(CHR);
	ui_.cb_chr2_sv2->addItems(CHR);
	ui_.cb_type_sv1->addItems(QStringList() << "DEL" << "DUP" << "INS" << "INV" << "BND");
	ui_.cb_type_sv2->addItems(QStringList() << "DEL" << "DUP" << "INS" << "INV" << "BND");

    // set date
    ui_.de_last_eval->setDate(QDate::currentDate());

    // set headers for disease info
    ui_.tw_disease_info->setColumnCount(2);
    ui_.tw_disease_info->setHorizontalHeaderItem(0, new QTableWidgetItem("type"));
	ui_.tw_disease_info->setHorizontalHeaderItem(1, new QTableWidgetItem("id"));
    ui_.tw_disease_info->setColumnWidth(0, 250);
    ui_.tw_disease_info->setColumnWidth(1, 150);

    // set defaults:
    ui_.cb_collection_method->setCurrentText("clinical testing");
	clinvar_upload_data_.submission_type = ClinvarSubmissionType::SingleVariant;
	ui_.remove_comphet_btn->setVisible(false);
	selectVariantType(0);

	//init sample selection:
	ui_.le_sample->fill(db_.createTable("sample_names", "SELECT id, name FROM sample"));

    //connect signal and slots
    connect(ui_.upload_btn, SIGNAL(clicked(bool)), this, SLOT(upload()));
	connect(ui_.add_comphet_btn, SIGNAL(clicked(bool)), this, SLOT(addCompHetVariant()));
	connect(ui_.remove_comphet_btn, SIGNAL(clicked(bool)), this, SLOT(removeCompHetVariant()));

	connect(ui_.cb_chr_snv1, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start_snv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end_snv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_ref_snv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_obs_snv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_genes_snv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));

	connect(ui_.cb_chr_snv2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start_snv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end_snv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_ref_snv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_obs_snv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));

	connect(ui_.cb_chr_cnv1, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start_cnv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end_cnv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_cn_cnv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_rcn_cnv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_genes_cnv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));

	connect(ui_.cb_chr_cnv2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start_cnv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end_cnv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_cn_cnv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_rcn_cnv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));

	connect(ui_.cb_type_sv1, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.cb_chr1_sv1, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start1_sv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end1_sv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.cb_chr2_sv1, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start2_sv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end2_sv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_genes_sv1, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));

	connect(ui_.cb_type_sv2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.cb_chr1_sv2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start1_sv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end1_sv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.cb_chr2_sv2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_start2_sv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_end2_sv2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));

	connect(ui_.le_sample, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.le_sample, SIGNAL(textEdited(QString)), this, SLOT(setDiseaseInfo()));
    connect(ui_.phenos, SIGNAL(phenotypeSelectionChanged()), this, SLOT(checkGuiData()));
	connect(ui_.tw_disease_info, SIGNAL(cellChanged(int,int)), this, SLOT(checkGuiData()));
    connect(ui_.print_btn, SIGNAL(clicked(bool)), this, SLOT(printResults()));
    connect(ui_.comment_upload, SIGNAL(textChanged()), this, SLOT(updatePrintButton()));
	connect(ui_.cb_var_type, SIGNAL(currentIndexChanged(int)), this, SLOT(selectVariantType(int)));

	//init disease add/remove
	connect(ui_.btn_delete_disease_info, SIGNAL(clicked(bool)), this, SLOT(removeDiseaseInfo()));
	QMenu* menu = new QMenu();
	menu->addAction("OMIM disease/phenotype identifier", this, SLOT(addDiseaseInfo()));
	menu->addAction("Orpha number", this, SLOT(addDiseaseInfo()));
	ui_.btn_add_disease_info->setMenu(menu);

	updateGUI();
}

void ClinvarUploadDialog::upload()
{
	// skip if already running
	if (upload_running_) return;
	upload_running_ = true;

	//deactivate upload button
	ui_.upload_btn->setEnabled(false);

    QJsonObject clinvar_submission = createJson();

	// write json to file
	if (test_run)
	{
		//get json file for debug
		QJsonDocument json_doc = QJsonDocument(clinvar_submission);
		QSharedPointer<QFile> json_file = Helper::openFileForWriting(clinvar_upload_data_.processed_sample + "_" + Helper::dateTime("yyyyMMdd_hhmmss") + ".json");
		json_file->write(json_doc.toJson());
		json_file->close();
	}

    QStringList errors;
    if (!validateJson(clinvar_submission, errors))
    {
        QMessageBox::warning(this, "JSON validation failed", "The generated JSON contains the following errors: \n" + errors.join("\n"));
		ui_.upload_btn->setEnabled(true);
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
	static HttpHandler http_handler(false); //static to allow caching of credentials
    try
    {
		//switch on/off testing
		if(test_run) qDebug() << "Test run enabled!";

        QStringList messages;
        messages << ui_.comment_upload->toPlainText();

        //add headers
        HttpHeaders add_headers;
        add_headers.insert("Content-Type", "application/json");
        add_headers.insert("SP-API-KEY", api_key);

        //post request
		QByteArray reply = http_handler.post(api_url, QJsonDocument(post_request).toJson(QJsonDocument::Compact), add_headers);

        // parse response
        bool success = false;
        QString submission_id;
        QJsonObject response = QJsonDocument::fromJson(reply).object();

        //successful dry-run
        if (response.isEmpty())
        {
			messages << "ERROR: API return empty response!";
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
			if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::SingleVariant)
			{
				details << "submission_type=SingleVariant";
			}
			else if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
			{
				details << "submission_type=CompoundHeterozygous";
			}
			else
			{
				THROW(ArgumentException, "Invalid submission type!")
			}
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
			GUIHelper::resizeTableCellWidths(ui_.tw_disease_info);
			GUIHelper::resizeTableCellHeightsToFirst(ui_.tw_disease_info);
            details << "condition=" + condition.join(',');
			if (manual_upload_)
			{
				details << "variant_type1=" + variantTypeToString(clinvar_upload_data_.variant_type1);
				switch (clinvar_upload_data_.variant_type1)
				{
					case VariantType::SNVS_INDELS:
						details << "variant_desc1=chr" + ui_.cb_chr_snv1->currentText() + ":" + ui_.le_start_snv1->text() + "-" + ui_.le_end_snv1->text()
								   + " " + ui_.le_ref_snv1->text() + ">" + ui_.le_obs_snv1->text();
						break;
					case VariantType::CNVS:
						details << "variant_desc1=chr" + ui_.cb_chr_cnv1->currentText() + ":" + ui_.le_start_cnv1->text() + "-" + ui_.le_end_cnv1->text()
								   + " cn:" + ui_.le_cn_cnv1->text() + " rcn:" + ui_.le_rcn_cnv1->text();
						break;
					case VariantType::SVS:
						details << "variant_desc1=" + ui_.cb_type_sv1->currentText() + " chr" + ui_.cb_chr1_sv1->currentText() + ":" + ui_.le_start1_sv1->text() + "-" + ui_.le_end1_sv1->text()
								   + " chr"+ ui_.cb_chr2_sv1->currentText() + ":" + ui_.le_start2_sv1->text() + "-" + ui_.le_end2_sv1->text();
						break;
					default:
						THROW(ArgumentException, "Invalid variant type provided!");
						break;
				}

				if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
				{
					details << "variant_type2=" + variantTypeToString(clinvar_upload_data_.variant_type2);
					switch (clinvar_upload_data_.variant_type2)
					{
						case VariantType::SNVS_INDELS:
							details << "variant_desc2=chr" + ui_.cb_chr_snv2->currentText() + ":" + ui_.le_start_snv2->text() + "-" + ui_.le_end_snv2->text()
									   + " " + ui_.le_ref_snv2->text() + ">" + ui_.le_obs_snv2->text();
							break;
						case VariantType::CNVS:
							details << "variant_desc2=chr" + ui_.cb_chr_cnv2->currentText() + ":" + ui_.le_start_cnv2->text() + "-" + ui_.le_end_cnv2->text()
									   + " cn:" + ui_.le_cn_cnv2->text() + " rcn:" + ui_.le_rcn_cnv2->text();
							break;
						case VariantType::SVS:
							details << "variant_desc2=" + ui_.cb_type_sv2->currentText() + " chr" + ui_.cb_chr1_sv2->currentText() + ":" + ui_.le_start1_sv2->text() + "-" + ui_.le_end1_sv2->text()
									   + " chr"+ ui_.cb_chr2_sv2->currentText() + ":" + ui_.le_start2_sv2->text() + "-" + ui_.le_end2_sv2->text();
							break;
						default:
							THROW(ArgumentException, "Invalid variant type provided!");
							break;
					}

				}
			}
			else
			{
				details << "variant_type1=" + variantTypeToString(clinvar_upload_data_.variant_type1);
				details << "variant_id1=" + QString::number(clinvar_upload_data_.variant_id1);
				details << "variant_rc_id1=" + getDbTableName(false) + ":" + QString::number(clinvar_upload_data_.report_variant_config_id1);
				if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
				{
					details << "variant_type2=" + variantTypeToString(clinvar_upload_data_.variant_type2);
					details << "variant_id2=" + QString::number(clinvar_upload_data_.variant_id2);
					details << "variant_rc_id2=" + getDbTableName(true) + ":" + QString::number(clinvar_upload_data_.report_variant_config_id2);
				}
			}

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
			QString genes;
			switch (clinvar_upload_data_.variant_type1)
			{
				case VariantType::SNVS_INDELS:
					genes = ui_.le_genes_snv1->text();
					break;
				case VariantType::CNVS:
					genes = ui_.le_genes_cnv1->text();
					break;
				case VariantType::SVS:
					genes = ui_.le_genes_sv1->text();
					break;
				default:
					THROW(ArgumentException, "Invalid variant type provided!");
					break;
			}
			details << "gene=" +  NGSD().genesToApproved(GeneSet::createFromStringList(genes.replace(";", ",").split(','))).toStringList().join(',');

			// additional info for reupload
			if (clinvar_upload_data_.variant_publication_id > 0)
			{
				details << "reupload=true";
				details << "previous_publication_id=" + QString::number(clinvar_upload_data_.variant_publication_id);
				details << "reupload_by=" + LoginManager::userLogin();
			}

			if (!test_run)
			{
				// log publication in NGSD
				if (manual_upload_)
				{
					int pub_id = db_.addManualVariantPublication(ui_.le_sample->text(), "ClinVar", convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), -1);
					if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
					{
						//switch variant 1 and 2
						QStringList details2;
						foreach (QString kv_pair, details)
						{
							if (kv_pair.startsWith("variant_type1=")) kv_pair.replace("variant_type1=", "variant_type2=");
							else if (kv_pair.startsWith("variant_type2=")) kv_pair.replace("variant_type2=", "variant_type1=");
							else if (kv_pair.startsWith("variant_desc1=")) kv_pair.replace("variant_desc1=", "variant_desc2=");
							else if (kv_pair.startsWith("variant_desc2=")) kv_pair.replace("variant_desc2=", "variant_desc1=");
							details2 << kv_pair;
						}
						int second_pub_id = db_.addManualVariantPublication(ui_.le_sample->text(), "ClinVar", convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details2.join(";"), -1);
						db_.linkVariantPublications(pub_id, second_pub_id);
					}
				}
				else
				{
					int pub_id;
					switch (clinvar_upload_data_.variant_type1)
					{
						case VariantType::SNVS_INDELS:
							pub_id =db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.snv1, "ClinVar",
															  convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), clinvar_upload_data_.user_id);
							break;
						case VariantType::CNVS:
							pub_id =db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.cnv1, "ClinVar",
															  convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), clinvar_upload_data_.user_id);
							break;
						case VariantType::SVS:
							pub_id =db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.sv1, GlobalServiceProvider::getSvList(),  "ClinVar",
													  convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), clinvar_upload_data_.user_id);
							break;
						default:
							THROW(ArgumentException, "Invalid variant type provided!");
							break;
					}
					if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
					{
						int second_pub_id;
						switch (clinvar_upload_data_.variant_type2)
						{
							case VariantType::SNVS_INDELS:
								second_pub_id = db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.snv2, "ClinVar",
																		  convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), clinvar_upload_data_.user_id);
								break;
							case VariantType::CNVS:
								second_pub_id = db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.cnv2, "ClinVar",
																		  convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), clinvar_upload_data_.user_id);
								break;
							case VariantType::SVS:
								second_pub_id = db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.sv2, GlobalServiceProvider::getSvList(),  "ClinVar",
														  convertClassification(ui_.cb_clin_sig_desc->currentText(), true), details.join(";"), clinvar_upload_data_.user_id);
								break;
							default:
								THROW(ArgumentException, "Invalid variant type provided!");
								break;
						}
						db_.linkVariantPublications(pub_id, second_pub_id);
					}
				}

				// for reupload: flag previous upload as replaced
				if (clinvar_upload_data_.variant_publication_id > 0) db_.flagVariantPublicationAsReplaced(clinvar_upload_data_.variant_publication_id);

				// for update: flag all over uploads with same SCV as replaced
				if (!clinvar_upload_data_.stable_id.trimmed().isEmpty())
				{
					//get variant publication id
					QList<int> pub_ids_to_replace = db_.getValuesInt("SELECT id FROM variant_publication WHERE replaced = 0 AND result LIKE :0", "%" + clinvar_upload_data_.stable_id + "%");
					foreach (int pub_id, pub_ids_to_replace)
					{
						db_.flagVariantPublicationAsReplaced(pub_id);
					}
				}
			}

            //show result
            QStringList lines;
            lines << "DATA UPLOAD TO CLINVAR SUCCESSFUL";
            lines << "";
            lines << messages.join("\n");
            lines << "";
			lines << "sample: " + ((manual_upload_)? ui_.le_sample->text(): clinvar_upload_data_.processed_sample);

			// log original submitter for reuploads
			if (clinvar_upload_data_.user_id > 0)
			{
				lines << "user: " + db_.userLogin(clinvar_upload_data_.user_id) + " (Reupload by " + LoginManager::userLogin() + ")";
			}
			else
			{
				lines << "user: " + LoginManager::userLogin();
			}
            lines << "date: " + Helper::dateTime();
            lines << "";
            lines << details;

            ui_.comment_upload->setText(lines.join("\n").replace("=", ": "));

			if (!test_run)
			{
				//write report file to transfer folder
				QString gsvar_publication_folder = Settings::path("gsvar_publication_folder");
				if (gsvar_publication_folder!="")
				{
						QString file_rep = gsvar_publication_folder + "/" + ((manual_upload_)? ui_.le_sample->text(): clinvar_upload_data_.processed_sample) + "_CLINVAR_"
								+ QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
						Helper::storeTextFile(file_rep, ui_.comment_upload->toPlainText().split("\n"));
				}
			}
			upload_running_ = false;
			ui_.upload_btn->setEnabled(false);

        }
        else
        {
            // Upload failed
			upload_running_ = false;
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

	//perform checks
	QStringList errors;
	bool var1_uploaded_to_clinvar = false;
	bool var2_uploaded_to_clinvar = false;
	QString upload_details_var1;
	QString upload_details_var2;
	GeneSet gene_set;

	//check if the given variants are manually edited and prevent upload in this case
	if(clinvar_upload_data_.report_variant_config1.isManuallyCurated() ||
	   (clinvar_upload_data_.submission_type==ClinvarSubmissionType::CompoundHeterozygous && clinvar_upload_data_.report_variant_config2.isManuallyCurated()))
	{
		errors << "The upload of manually curated variants is not possible with the standard ClinVar upload. Please use the manual upload instead.";
	}

	//check variant 1
	if(clinvar_upload_data_.variant_type1 == VariantType::SNVS_INDELS)
	{
		//check if already published
		if (clinvar_upload_data_.processed_sample !="" && clinvar_upload_data_.snv1.isValid())
		{
			upload_details_var1 = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.snv1);
		}

		// check chromosome
		if (ui_.cb_chr_snv1->currentText().trimmed().isEmpty())
		{
			errors << "Chromosome unset!";
		}

		// check start/end
		int int_value = 0;
		bool ok = true;
		int_value = ui_.le_start_snv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid variant start pos '" + ui_.le_start_snv1->text() + "'!";
		}
		int_value = ui_.le_end_snv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid variant end pos '" + ui_.le_end_snv1->text() + "'!";
		}

		// check sequences
		QRegExp re("[-]|[ACGTU]*");
		if (!re.exactMatch(ui_.le_ref_snv1->text()))
		{
			errors << "invalid reference sequence '" + ui_.le_ref_snv1->text() + "'!";
		}
		if (!re.exactMatch(ui_.le_obs_snv1->text()))
		{
			errors << "invalid observed sequence '" + ui_.le_obs_snv1->text() + "'!";
		}

		//check genes
		gene_set = GeneSet::createFromStringList(ui_.le_genes_snv1->text().split(','));
	}
	else if(clinvar_upload_data_.variant_type1 == VariantType::CNVS)
	{
		//check if already published
		if (clinvar_upload_data_.processed_sample !="")
		{
			upload_details_var1 = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.cnv1);
		}

		// check chromosome
		if (ui_.cb_chr_cnv1->currentText().trimmed().isEmpty())
		{
			errors << "Chromosome unset!";
		}

		// check start/end
		int int_value = 0;
		bool ok = true;
		int_value = ui_.le_start_cnv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid CNV start pos '" + ui_.le_start_cnv1->text() + "'!";
		}
		int_value = ui_.le_end_cnv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid CNV end pos '" + ui_.le_end_cnv1->text() + "'!";
		}

		// check copy number
		int_value = ui_.le_cn_cnv1->text().toInt(&ok);
		if ( !ok || (int_value < 0))
		{
			errors << "Invalid CNV copy number '" + ui_.le_cn_cnv1->text() + "'!";
		}
		int_value = ui_.le_rcn_cnv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid CNV reference copy number '" + ui_.le_rcn_cnv1->text() + "'!";
		}


		//check genes
		gene_set = GeneSet::createFromStringList(ui_.le_genes_cnv1->text().split(','));
	}
	else if(clinvar_upload_data_.variant_type1 == VariantType::SVS)
	{
		//check if already published
		if (clinvar_upload_data_.processed_sample !="")
		{
			upload_details_var1 = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.sv1, GlobalServiceProvider::getSvList());
		}

		//check SV type
		if (ui_.cb_type_sv1->currentText() == "BND")
		{
			errors << "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.";
		}

		// check chromosome
		if (ui_.cb_chr1_sv1->currentText().trimmed().isEmpty())
		{
			errors << "Chromosome 1 unset!";
		}
		if (ui_.cb_chr2_sv1->currentText().trimmed().isEmpty())
		{
			errors << "Chromosome 2 unset!";
		}

		// check start/end
		int int_value = 0;
		bool ok = true;
		int_value = ui_.le_start1_sv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid SV start1 pos '" + ui_.le_start1_sv1->text() + "'!";
		}
		int_value = ui_.le_start2_sv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid SV start2 pos '" + ui_.le_start2_sv1->text() + "'!";
		}
		int_value = ui_.le_end1_sv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid SV end1 pos '" + ui_.le_end1_sv1->text() + "'!";
		}
		int_value = ui_.le_end2_sv1->text().toInt(&ok);
		if ( !ok || (int_value < 1))
		{
			errors << "Invalid SV end2 pos '" + ui_.le_end2_sv1->text() + "'!";
		}

		//check genes
		gene_set = GeneSet::createFromStringList(ui_.le_genes_sv1->text().split(','));
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type provided for variant 1!")
	}

	// check if variant is already uploaded
	if (upload_details_var1!="")
	{
		//check if uploaded to Clinvar
		foreach (const QString& line, upload_details_var1.split('\n'))
		{
			QStringList columns = line.split(' ');
			for (int i = 0; i < (columns.size()-1); ++i)
			{
				if (columns[i] == "db:")
				{
					if (columns[i+1] == "ClinVar")
					{
						// already uploaded to ClinVar
						var1_uploaded_to_clinvar = true;
					}
					break;
				}
			}
			// shortcut
			if (var1_uploaded_to_clinvar) break;
		}
	}

	//check genes
	QStringList invalid_genes;
	foreach (QByteArray gene, gene_set)
	{
		gene = gene.trimmed();
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

	//check variant 2 (comp-het)
	if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if(clinvar_upload_data_.variant_type2 == VariantType::SNVS_INDELS)
		{
			//check if already published
			if (clinvar_upload_data_.processed_sample != "" && clinvar_upload_data_.snv2.isValid())
			{
				upload_details_var2 = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.snv2);
			}

			//perform checks
			QStringList errors;
			// check chromosome
			if (ui_.cb_chr_snv2->currentText().trimmed().isEmpty())
			{
				errors << "Chromosome for variant 2 unset!";
			}

			// check start/end
			int int_value = 0;
			bool ok = true;
			int_value = ui_.le_start_snv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid variant 2 start pos '" + ui_.le_start_snv2->text() + "'!";
			}
			int_value = ui_.le_end_snv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid variant 2 end pos '" + ui_.le_end_snv2->text() + "'!";
			}

			// check sequences
			QRegExp re("[-]|[ACGTU]*");
			if (!re.exactMatch(ui_.le_ref_snv2->text()))
			{
				errors << "invalid reference sequence '" + ui_.le_ref_snv2->text() + "'!";
			}
			if (!re.exactMatch(ui_.le_obs_snv2->text()))
			{
				errors << "invalid observed sequence '" + ui_.le_obs_snv2->text() + "'!";
			}

		}
		else if(clinvar_upload_data_.variant_type2 == VariantType::CNVS)
		{
			//check if already published
			if (clinvar_upload_data_.processed_sample !="")
			{
				upload_details_var2 = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.cnv2);
			}

			int int_value = 0;
			bool ok = true;

			// check chromosome
			if (ui_.cb_chr_cnv2->currentText().trimmed().isEmpty())
			{
				errors << "Chromosome unset!";
			}

			// check start/end
			int_value = ui_.le_start_cnv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid CNV start position '" + ui_.le_start_cnv2->text() + "' for CNV 2!";
			}
			int_value = ui_.le_end_cnv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid CNV end position '" + ui_.le_end_cnv2->text() + "' for CNV 2!";
			}

			// check copy number
			int_value = ui_.le_cn_cnv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid CNV copy number '" + ui_.le_cn_cnv2->text() + "' for CNV 2!";
			}
			int_value = ui_.le_rcn_cnv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid CNV reference copy number '" + ui_.le_rcn_cnv2->text() + "' for CNV 2!";
			}

		}
		else if(clinvar_upload_data_.variant_type2 == VariantType::SVS)
		{
			//check if already published
			if (clinvar_upload_data_.processed_sample !="")
			{
				upload_details_var2 = db_.getVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.sv2, GlobalServiceProvider::getSvList());
			}

			//check SV type
			if (ui_.cb_type_sv2->currentText() == "BND")
			{
				errors << "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.";
			}

			// check chromosome
			if (ui_.cb_chr1_sv2->currentText().trimmed().isEmpty())
			{
				errors << "Chromosome 1  of SV 2 unset!";
			}
			if (ui_.cb_chr2_sv2->currentText().trimmed().isEmpty())
			{
				errors << "Chromosome 2 of SV 2 unset!";
			}

			// check start/end
			int int_value = 0;
			bool ok = true;
			int_value = ui_.le_start1_sv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid SV start1 position '" + ui_.le_start1_sv2->text() + "' for SV 2!";
			}
			int_value = ui_.le_start2_sv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid SV start2 position '" + ui_.le_start2_sv2->text() + "' for SV 2!";
			}
			int_value = ui_.le_end1_sv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid SV end1 pos '" + ui_.le_end1_sv2->text() + "' for SV 2!";
			}
			int_value = ui_.le_end2_sv2->text().toInt(&ok);
			if ( !ok || (int_value < 1))
			{
				errors << "Invalid SV end2 pos '" + ui_.le_end2_sv2->text() + "' for SV 2!";
			}

		}
		else
		{
			THROW(ArgumentException, "Invalid variant type provided for variant 2!")
		}
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

	// check disease info
	if (ui_.tw_disease_info->rowCount() == 0)
	{
		errors << "No disease ID provided!";
	}
	if (ui_.tw_disease_info->rowCount() > 1)
	{
		errors << "More than one disease ID provided - please delete all but one through the context menu!";
	}

	// check sample name (only for manual upload)
	if (manual_upload_ && db_.sampleId(ui_.le_sample->text(), false).isEmpty())
	{
		errors << "No valid sample name provided!";
	}

	QStringList upload_comment_text;
	if (var1_uploaded_to_clinvar)
	{
		//extract SCV:
		if(clinvar_upload_data_.stable_id.trimmed().isEmpty())
		{
			foreach (const QString& line, upload_details_var1.split('\n'))
			{
				if(clinvar_upload_data_.stable_id.trimmed().isEmpty() && line.startsWith("result: processed, SCV"))
				{
					clinvar_upload_data_.stable_id = line.split(", ").at(1).trimmed();
					break;
				}
			}
		}
		upload_comment_text << QString("<font color='red'>WARNING: This variant has already been uploaded to ClinVar! Are you sure you want to upload it again? ")
							   + ((clinvar_upload_data_.stable_id.isEmpty())?"":"The variant will be replaced on ClinVar. ")
							   + "</font><br>"
							   + upload_details_var1.replace("\n", "<br>").replace("%20", " ");
	}
	if (var2_uploaded_to_clinvar)
	{
		//extract SCV:
		if(clinvar_upload_data_.stable_id.trimmed().isEmpty())
		{
			foreach (const QString& line, upload_details_var2.split('\n'))
			{
				if(clinvar_upload_data_.stable_id.trimmed().isEmpty() && line.startsWith("result: processed, SCV"))
				{
					clinvar_upload_data_.stable_id = line.split(", ").at(1).trimmed();
					break;
				}
			}
		}
		upload_comment_text << QString("<font color='red'>WARNING: The second variant has already been uploaded to ClinVar! Are you sure you want to upload it again? ")
							   + ((clinvar_upload_data_.stable_id.isEmpty())?"":"The variant will be replaced on ClinVar. ")
							   + "</font><br>"
							   + upload_details_var2.replace("\n", "<br>").replace("%20", " ");
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

void ClinvarUploadDialog::addCompHetVariant()
{
	if (manual_upload_)
	{
		QComboBox* variant_type_selector = new QComboBox(this);
		variant_type_selector->addItems(QStringList() << "SNV/INDEL" << "CNV" << "SV");

		auto dlg = GUIHelper::createDialog(variant_type_selector, "Compound heterozygous variant", "Select type of compound heterozygous variant: ", true);
		int btn = dlg->exec();
		if (btn == 1)
		{
			ui_.sw_var2->setCurrentIndex(variant_type_selector->currentIndex());
			clinvar_upload_data_.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
			switch (variant_type_selector->currentIndex())
			{
				case 0:
					clinvar_upload_data_.variant_type2 = VariantType::SNVS_INDELS;
					break;
				case 1:
					clinvar_upload_data_.variant_type2 = VariantType::CNVS;
					break;
				case 2:
					clinvar_upload_data_.variant_type2 = VariantType::SVS;
					break;
				default:
					clinvar_upload_data_.variant_type2 = VariantType::INVALID;
					THROW(ArgumentException, "Invalid variant type selected!")
					break;
			}
			updateGUI();
		}
	}
	else
	{
		ReportVariantSelectionDialog dialog(db_.processedSampleId(clinvar_upload_data_.processed_sample), clinvar_upload_data_.report_variant_config_id1, this);

		if (dialog.exec()==QDialog::Accepted)
		{
			// parse selected variant
			SelectedReportVariant report_variant = dialog.getSelectedReportVariant();

			clinvar_upload_data_.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
			clinvar_upload_data_.variant_type2 = report_variant.report_variant_configuration.variant_type;
			clinvar_upload_data_.report_variant_config_id2 = report_variant.rvc_id;
			if (clinvar_upload_data_.variant_type2 == VariantType::SNVS_INDELS)
			{
				clinvar_upload_data_.snv2 = report_variant.small_variant;
				clinvar_upload_data_.variant_id2 = report_variant.variant_id;
			}
			else if(clinvar_upload_data_.variant_type2 ==  VariantType::CNVS)
			{
				clinvar_upload_data_.cnv2 = report_variant.cnv;
				clinvar_upload_data_.cn2 = report_variant.cn;
				clinvar_upload_data_.ref_cn2 = report_variant.ref_cn;
				clinvar_upload_data_.variant_id2 = report_variant.variant_id;
			}
			else if(clinvar_upload_data_.variant_type2 == VariantType::SVS)
			{
				clinvar_upload_data_.sv2 = report_variant.sv;
				clinvar_upload_data_.variant_id2 = report_variant.variant_id;
			}


			setData(clinvar_upload_data_);
		}
	}
	checkGuiData();
}

void ClinvarUploadDialog::removeCompHetVariant()
{
	if(clinvar_upload_data_.submission_type != ClinvarSubmissionType::CompoundHeterozygous) return;

	clinvar_upload_data_.submission_type = ClinvarSubmissionType::SingleVariant;
	clinvar_upload_data_.variant_type2 = VariantType::INVALID;
	clinvar_upload_data_.variant_id2 = -1;
	clinvar_upload_data_.report_variant_config_id2 = -1;
	clinvar_upload_data_.cn2 = -1;
	clinvar_upload_data_.snv2 = Variant();
	clinvar_upload_data_.cnv2 = CopyNumberVariant();
	clinvar_upload_data_.sv2 = BedpeLine();

	if (!manual_upload_) setData(clinvar_upload_data_);
	updateGUI();
	checkGuiData();
}

void ClinvarUploadDialog::selectVariantType(int i)
{
	ui_.sw_var1->setCurrentIndex(i);
	// set variant type
	switch (i) {
		case 0:
			clinvar_upload_data_.variant_type1 = VariantType::SNVS_INDELS;
			break;
		case 1:
			clinvar_upload_data_.variant_type1 = VariantType::CNVS;
			break;
		case 2:
			clinvar_upload_data_.variant_type1 = VariantType::SVS;
			break;
		default:
			clinvar_upload_data_.variant_type1 = VariantType::INVALID;
			THROW(ArgumentException, "Invalid variant type selected!");
			break;
	}
	checkGuiData();
}

void ClinvarUploadDialog::updateGUI()
{
	// switch comp-het variant on/off
	ui_.sw_var2->setVisible(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous);
	ui_.sw_var2->setEnabled(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous);
	ui_.l_comp_het->setVisible(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous);
	ui_.remove_comphet_btn->setVisible(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous);
	ui_.remove_comphet_btn->setEnabled(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous);
	ui_.add_comphet_btn->setVisible(clinvar_upload_data_.submission_type == ClinvarSubmissionType::SingleVariant);
	ui_.add_comphet_btn->setEnabled(clinvar_upload_data_.submission_type == ClinvarSubmissionType::SingleVariant);

	//disable variant selection
	ui_.w_var_type->setVisible(manual_upload_);
	ui_.w_var_type->setEnabled(manual_upload_);

	//disable disease modification
	ui_.btn_add_disease_info->setVisible(manual_upload_);
	ui_.btn_add_disease_info->setEnabled(manual_upload_);
	ui_.btn_delete_disease_info->setVisible(manual_upload_);
	ui_.btn_delete_disease_info->setEnabled(manual_upload_);
}

void ClinvarUploadDialog::setDiseaseInfo()
{
	if (!manual_upload_) return;
	QString s_id = db_.sampleId(ui_.le_sample->text(), false);
	if (s_id.isEmpty()) return;

	//get disease info from NGSD
	QList<SampleDiseaseInfo> disease_info = db_.getSampleDiseaseInfo(s_id);

	// set disease info
	ui_.tw_disease_info->setRowCount(disease_info.length());
	int row_idx = 0;
	foreach (const SampleDiseaseInfo& disease, disease_info)
	{
		if ((disease.type == "OMIM disease/phenotype identifier") || (disease.type == "Orpha number"))
		{
			ui_.tw_disease_info->setItem(row_idx, 0, new QTableWidgetItem(disease.type));
			ui_.tw_disease_info->setItem(row_idx, 1, new QTableWidgetItem(disease.disease_info));
			row_idx++;
		}
	}
	ui_.tw_disease_info->setRowCount(row_idx);
}

void ClinvarUploadDialog::addDiseaseInfo()
{
	QString type = qobject_cast<QAction*>(sender())->text();

	QString text = QInputDialog::getText(this, "Add disease info", type);
	if (text.isEmpty()) return;

	//check if valid
	if (type=="OMIM disease/phenotype identifier" && !QRegExp("#\\d*").exactMatch(text))
	{
		QMessageBox::critical(this, "Invalid OMIM identifier", "OMIM disease/phenotype identifier invaid!\nA valid identifier is for example '#164400'.");
		return;
	}
	if (type=="Orpha number" && !QRegExp("ORPHA:\\d*").exactMatch(text))
	{
		QMessageBox::critical(this, "Invalid Orpha number", "Orpha number invaid!\nA valid number is for example 'ORPHA:1172'.");
		return;
	}

	int row_idx = ui_.tw_disease_info->rowCount();
	ui_.tw_disease_info->setRowCount(row_idx+1);
	ui_.tw_disease_info->setItem(row_idx, 0, GUIHelper::createTableItem(type));
	ui_.tw_disease_info->setItem(row_idx, 1, GUIHelper::createTableItem(text));

	GUIHelper::resizeTableCellWidths(ui_.tw_disease_info);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.tw_disease_info);
}

void ClinvarUploadDialog::removeDiseaseInfo()
{
	QList<QTableWidgetSelectionRange> ranges = ui_.tw_disease_info->selectedRanges();

	if (ranges.count()!=1) return;
	if (ranges[0].topRow()!=ranges[0].bottomRow()) return;

	int row = ranges[0].topRow();
	ui_.tw_disease_info->removeRow(row);
}

void ClinvarUploadDialog::diseaseContextMenu(QPoint pos)
{
	//make sure only one row is selected
	QList<int> selected_rows = GUIHelper::selectedTableRows(ui_.tw_disease_info);
	if (selected_rows.count()!=1) return;
	int row = selected_rows[0];

	//create menu
	QMenu menu;
	QAction* action_del = new QAction(QIcon(":/Icons/Delete.png"), "Delete");
	menu.addAction(action_del);

	QAction* action_open = new QAction(QIcon(":/Icons/Link.png"), "Open external database (if available)", this);
	menu.addAction(action_open);

	//execute
	QAction* action = menu.exec(ui_.tw_disease_info->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	//perform actions
	if (action==action_del)
	{
		ui_.tw_disease_info->removeRow(row);
		checkGuiData();
	}
	if (action==action_open)
	{
		QString value = ui_.tw_disease_info->item(row, 1)->text().trimmed();

		QString link;
		if (value.startsWith("#"))
		{
			value.replace("#", ""); //remove prefix
			link = "https://omim.org/entry/" + value;
		}
		else if (value.startsWith("ORPHA:"))
		{
			value.replace("ORPHA:", ""); //remove prefix
			link = "https://www.orpha.net/consor/cgi-bin/OC_Exp.php?lng=en&Expert=" + value;
		}

		if (!link.isEmpty())
		{
			QDesktopServices::openUrl(QUrl(link));
		}
	}
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

	//optional
	//json.insert("clinvarDeletion", "");

    //required
    QJsonObject clinvar_submission;
    {
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
		if (!clinvar_upload_data_.stable_id.isEmpty())
		{
			clinvar_submission.insert("clinvarAccession", clinvar_upload_data_.stable_id);
		}

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

		// only for upload from NGSD variants:
		if (!manual_upload_)
		{
			//optional
			clinvar_submission.insert("localID", QString::number(clinvar_upload_data_.variant_id1));
			//optional
			clinvar_submission.insert("localKey", getDbTableName(false) + ":" + QString::number(clinvar_upload_data_.report_variant_config_id1));
		}

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
					feature.insert("clinicalFeaturesAffectedStatus", "present");
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
		clinvar_submission.insert("recordStatus", (clinvar_upload_data_.stable_id.isEmpty())?"novel":"update");

        //required
		QJsonObject variant_set1;
        {
            QJsonArray variants;
            {
				//variant 1
				QJsonObject variant1;
                {
					QString gene_string;

					if (clinvar_upload_data_.variant_type1 == VariantType::SNVS_INDELS)
					{
						//required (except hgvs)
						QJsonObject chromosome_coordinates;
						{
							chromosome_coordinates.insert("alternateAllele", ui_.le_obs_snv1->text());
							chromosome_coordinates.insert("assembly", buildToString(GSvarHelper::build(), true));
							chromosome_coordinates.insert("chromosome", ui_.cb_chr_snv1->currentText());
							chromosome_coordinates.insert("referenceAllele", ui_.le_ref_snv1->text());
							chromosome_coordinates.insert("start", Helper::toInt(ui_.le_start_snv1->text()));
							chromosome_coordinates.insert("stop", Helper::toInt(ui_.le_end_snv1->text()));
						}
						variant1.insert("chromosomeCoordinates", chromosome_coordinates);

						//optional (but required for deletions and insertions)
						QString variant_type;
						if (ui_.le_ref_snv1->text().contains("-") || ui_.le_ref_snv1->text().trimmed().isEmpty()) variant_type = "Insertion";
						if (ui_.le_obs_snv1->text().contains("-") || ui_.le_obs_snv1->text().trimmed().isEmpty()) variant_type = "Deletion";
						if (!variant_type.isEmpty()) variant1.insert("variantType", variant_type);

						//parse genes
						gene_string = ui_.le_genes_snv1->text();
					}
					else if (clinvar_upload_data_.variant_type1 == VariantType::CNVS)
					{
						//required (except hgvs)
						QJsonObject chromosome_coordinates;
						{
							chromosome_coordinates.insert("assembly", buildToString(GSvarHelper::build(), true));
							chromosome_coordinates.insert("chromosome", ui_.cb_chr_cnv1->currentText());
							chromosome_coordinates.insert("start", Helper::toInt(ui_.le_start_cnv1->text()));
							chromosome_coordinates.insert("stop", Helper::toInt(ui_.le_end_cnv1->text()));
						}
						variant1.insert("chromosomeCoordinates", chromosome_coordinates);

						//optional (but required for CNVS)
						variant1.insert("copyNumber", ui_.le_cn_cnv1->text());
						variant1.insert("referenceCopyNumber", Helper::toInt(ui_.le_rcn_cnv1->text()));
						variant1.insert("variantType", (ui_.le_cn_cnv1->text().toInt() > ui_.le_rcn_cnv1->text().toInt())? "copy number gain": "copy number loss");

						//parse genes
						gene_string = ui_.le_genes_cnv1->text();
					}
					else if (clinvar_upload_data_.variant_type1 == VariantType::SVS)
					{
						//required (except hgvs)
						QJsonObject chromosome_coordinates;
						{
							chromosome_coordinates.insert("assembly", buildToString(GSvarHelper::build(), true));
							chromosome_coordinates.insert("chromosome", ui_.cb_chr1_sv1->currentText());

							if (ui_.cb_type_sv1->currentText() == "BND") THROW(NotImplementedException, "The upload of translocations is not supported by the ClinVar API!")

							int start1 = Helper::toInt(ui_.le_start1_sv1->text());
							int end1 = Helper::toInt(ui_.le_end1_sv1->text());
							int start2 = Helper::toInt(ui_.le_start2_sv1->text());
							int end2 = Helper::toInt(ui_.le_end2_sv1->text());

							if (start1 == end1)
							{
								chromosome_coordinates.insert("start", start1);
							}
							else
							{
								chromosome_coordinates.insert("outerStart", start1);
								chromosome_coordinates.insert("innerStart", end1);
							}
							if (start2 == end2)
							{
								chromosome_coordinates.insert("stop", start2);
							}
							else
							{
								chromosome_coordinates.insert("innerStop", start2);
								chromosome_coordinates.insert("outerStop", end2);
							}

						}
						variant1.insert("chromosomeCoordinates", chromosome_coordinates);

						//optional (but required for SVS)
						QString variant_type;
						switch (BedpeFile::stringToType(ui_.cb_type_sv1->currentText().toUtf8()))
						{
							case StructuralVariantType::DEL:
								variant_type = "Deletion";
								break;
							case StructuralVariantType::DUP:
								variant_type = "Duplication";
								break;
							case StructuralVariantType::INS:
								variant_type = "Insertion";
								break;
							case StructuralVariantType::INV:
								variant_type = "Inversion";
								break;
							case StructuralVariantType::BND:
								//TODO: implement for BND
								THROW(NotImplementedException, "Currently not implemented for translocations!");
								variant_type = "Translocation";
								break;
							default:
								break;
						}
						if (variant_type.isEmpty()) THROW(ArgumentException, "No valid SV type provided!");
						variant1.insert("variantType", variant_type);


						//parse genes
						gene_string = ui_.le_genes_sv1->text();
					}
					else
					{
						THROW(ArgumentException, "Invalid variant type provided!");
					}

					//optional
					if (!gene_string.trimmed().isEmpty())
					{
						QJsonArray genes;
						{
							GeneSet gene_set = NGSD().genesToApproved(GeneSet::createFromStringList(gene_string.replace(";", ",").split(',')));
							foreach (const QByteArray& gene_name, gene_set)
							{
								QJsonObject gene;
								gene.insert("symbol", QString(gene_name));
								genes.append(gene);
							}
						}
						variant1.insert("gene", genes);
					}

                }
				variants.append(variant1);

				}
			variant_set1.insert("variant", variants);
        }
		if(clinvar_upload_data_.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			//2nd variant for compound-heterozygote variant
			QJsonObject variant_set2;
			{
				QJsonArray variants;
				{
					//variant 2
					QJsonObject variant2;
					{
						QString gene_string;

						if (clinvar_upload_data_.variant_type2 == VariantType::SNVS_INDELS)
						{
							//required (except hgvs)
							QJsonObject chromosome_coordinates;
							{
								chromosome_coordinates.insert("alternateAllele", ui_.le_obs_snv2->text());
								chromosome_coordinates.insert("assembly", buildToString(GSvarHelper::build(), true));
								chromosome_coordinates.insert("chromosome", ui_.cb_chr_snv2->currentText());
								chromosome_coordinates.insert("referenceAllele", ui_.le_ref_snv2->text());
								chromosome_coordinates.insert("start", Helper::toInt(ui_.le_start_snv2->text()));
								chromosome_coordinates.insert("stop", Helper::toInt(ui_.le_end_snv2->text()));
							}
							variant2.insert("chromosomeCoordinates", chromosome_coordinates);

							//optional (but required for deletions and insertions)
							QString variant_type;
							if (ui_.le_ref_snv2->text().contains("-") || ui_.le_ref_snv2->text().trimmed().isEmpty()) variant_type = "Insertion";
							if (ui_.le_obs_snv2->text().contains("-") || ui_.le_obs_snv2->text().trimmed().isEmpty()) variant_type = "Deletion";
							if (!variant_type.isEmpty()) variant2.insert("variantType", variant_type);

							//parse genes
							gene_string = ui_.le_genes_snv1->text();
						}
						else if (clinvar_upload_data_.variant_type2 == VariantType::CNVS)
						{
							//required (except hgvs)
							QJsonObject chromosome_coordinates;
							{
								chromosome_coordinates.insert("assembly", buildToString(GSvarHelper::build(), true));
								chromosome_coordinates.insert("chromosome", ui_.cb_chr_cnv2->currentText());
								chromosome_coordinates.insert("start", Helper::toInt(ui_.le_start_cnv2->text()));
								chromosome_coordinates.insert("stop", Helper::toInt(ui_.le_end_cnv2->text()));
							}
							variant2.insert("chromosomeCoordinates", chromosome_coordinates);

							//optional (but required for CNVS)
							variant2.insert("copyNumber", ui_.le_cn_cnv2->text());
							variant2.insert("referenceCopyNumber", Helper::toInt(ui_.le_rcn_cnv2->text()));
							variant2.insert("variantType", (ui_.le_cn_cnv2->text().toInt() > ui_.le_rcn_cnv2->text().toInt())? "copy number gain": "copy number loss");

							//parse genes
							gene_string = ui_.le_genes_cnv1->text();
						}
						else if (clinvar_upload_data_.variant_type2 == VariantType::SVS)
						{
							//required (except hgvs)
							QJsonObject chromosome_coordinates;
							{
								chromosome_coordinates.insert("assembly", buildToString(GSvarHelper::build(), true));
								chromosome_coordinates.insert("chromosome", ui_.cb_chr1_sv2->currentText());

								if (ui_.cb_type_sv2->currentText() == "BND") THROW(NotImplementedException, "The upload of translocations is not supported by the ClinVar API!")

								int start1 = Helper::toInt(ui_.le_start1_sv2->text());
								int end1 = Helper::toInt(ui_.le_end1_sv2->text());
								int start2 = Helper::toInt(ui_.le_start2_sv2->text());
								int end2 = Helper::toInt(ui_.le_end2_sv2->text());

								if (start1 == end1)
								{
									chromosome_coordinates.insert("start", start1);
								}
								else
								{
									chromosome_coordinates.insert("outerStart", start1);
									chromosome_coordinates.insert("innerStart", end1);
								}
								if (start2 == end2)
								{
									chromosome_coordinates.insert("stop", start2);
								}
								else
								{
									chromosome_coordinates.insert("innerStop", start2);
									chromosome_coordinates.insert("outerStop", end2);
								}
							}
							variant2.insert("chromosomeCoordinates", chromosome_coordinates);

							//optional (but required for SVS)
							QString variant_type;
							switch (BedpeFile::stringToType(ui_.cb_type_sv2->currentText().toUtf8()))
							{
								case StructuralVariantType::DEL:
									variant_type = "Deletion";
									break;
								case StructuralVariantType::DUP:
									variant_type = "Duplication";
									break;
								case StructuralVariantType::INS:
									variant_type = "Insertion";
									break;
								case StructuralVariantType::INV:
									variant_type = "Inversion";
									break;
								case StructuralVariantType::BND:
									//TODO: implement for BND
									THROW(NotImplementedException, "Currently not implemented for translocations!");
									variant_type = "Translocation";
									break;
								default:
									break;
							}
							if (variant_type.isEmpty()) THROW(ArgumentException, "No valid SV type provided!");
							variant2.insert("variantType", variant_type);

							//parse genes
							gene_string = ui_.le_genes_sv1->text();
						}
						else
						{
							THROW(ArgumentException, "Invalid variant type 2 provided!");
						}

						//optional
						if (!gene_string.trimmed().isEmpty())
						{
							QJsonArray genes;
							{
								GeneSet gene_set = NGSD().genesToApproved(GeneSet::createFromStringList(ui_.le_genes_snv1->text().replace(";", ",").split(',')));
								foreach (const QByteArray& gene_name, gene_set)
								{
									QJsonObject gene;
									gene.insert("symbol", QString(gene_name));
									genes.append(gene);
								}
							}
							variant2.insert("gene", genes);
						}

					}
					variants.append(variant2);
				}
				variant_set2.insert("variant", variants);
			}

			QJsonObject compound_heterozygote_set;

			QString hgvs = getHGVS();
			compound_heterozygote_set.insert("hgvs", hgvs);

			QJsonArray variant_sets;
			QJsonObject var_set_item1;
			var_set_item1.insert("variantSet", variant_set1);
			variant_sets.append(var_set_item1);
			QJsonObject var_set_item2;
			var_set_item2.insert("variantSet", variant_set2);
			variant_sets.append(var_set_item2);
			compound_heterozygote_set.insert("variantSets", variant_sets);
			clinvar_submission.insert("compoundHeterozygoteSet", compound_heterozygote_set);
		}
		else
		{
			//single variant case
			clinvar_submission.insert("variantSet", variant_set1);
		}

    }
    json.insert("clinvarSubmission", QJsonArray() << clinvar_submission);

    //optional
    //json.insert("submissionName", "");

	//required
	json.insert("clinvarSubmissionReleaseStatus", ui_.cb_release_status->currentText());

	//optional
	QJsonObject assertion_criteria;
	{
		// add hardcoded citation
		assertion_criteria.insert("db", "PubMed");
		assertion_criteria.insert("id", "25741868");
//		assertion_criteria.insert("url", "https://pubmed.ncbi.nlm.nih.gov/25741868/");
	}
	json.insert("assertionCriteria", assertion_criteria);

	//optional
	//json.insert("behalfOrgID", "");

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

	if (clinvar_submission.contains("clinvarAccession"))
	{
		//parse optional ClinVar accession
		QString scv_id = clinvar_submission.value("clinvarAccession").toString();
		if (!scv_id.startsWith("SCV"))
		{
			errors << "ID '" + scv_id + "' for 'clinvarAccession' in 'clinvarSubmission' doesn't match the required format (Has to start with SCV)!";
			is_valid = false;
		}
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
		if (clinvar_submission.contains("compoundHeterozygoteSet"))
		{
			errors << "Only one of the entries 'variantSet' or 'compoundHeterozygoteSet' is allowed in one submission!";
			is_valid = false;
		}
        //parse variantSet
        QJsonObject variant_set = clinvar_submission.value("variantSet").toObject();
        if (variant_set.contains("variant"))
        {
            QJsonArray variant_array = variant_set.value("variant").toArray();
            if (variant_array.size() > 0)
            {
                foreach (const QJsonValue& variant, variant_array)
                {
					VariantType type = VariantType::SNVS_INDELS;

					if (variant.toObject().contains("variantType"))
					{
						QString variant_type = variant.toObject().value("variantType").toString();
						if (!VARIANT_TYPE.contains(variant_type))
						{
							errors << "Invalid variantType '" + variant_type + "'!";
							is_valid = false;
						}
						QStringList cnv_types;
						cnv_types << "copy number loss" << "copy number gain";
						QStringList sv_types;
						sv_types << "Insertion" << "Deletion" << "Duplication" << "Tandem duplication" << "Inversion" << "Translocation";
						if(cnv_types.contains(variant_type))
						{
							type = VariantType::CNVS;
						}
						else if(sv_types.contains(variant_type))
						{
							type = VariantType::SVS;
						}
						else
						{
							errors << "Invalid variantType '" + variant_type + "'!";
							is_valid = false;
						}
					}

                    if (variant.toObject().contains("chromosomeCoordinates"))
                    {
                        QJsonObject chromosome_coordinates = variant.toObject().value("chromosomeCoordinates").toObject();

						if (type == VariantType::SNVS_INDELS)
						{
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
								int valid_int = chromosome_coordinates.value("start").toInt(-1);
								if (valid_int < 0)
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
								int valid_int = chromosome_coordinates.value("stop").toInt(-1);
								if (valid_int < 0)
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
						else if (type == VariantType::CNVS)
						{
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
								int valid_int = chromosome_coordinates.value("start").toInt(-1);
								if (valid_int < 0)
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
								int valid_int = chromosome_coordinates.value("stop").toInt(-1);
								if (valid_int < 0)
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

							if (variant.toObject().contains("copyNumber"))
							{
								QString copy_number_string = variant.toObject().value("copyNumber").toString().trimmed();
								bool ok = false;
								int copy_number = copy_number_string.toInt(&ok);

								if(!ok || copy_number < 0)
								{
									errors << "Invalid entry '" + copy_number_string + "' in 'copyNumber'!";
									is_valid = false;
								}
							}
							else
							{
								errors << "Required string 'copyNumber' in 'variant' missing!";
								is_valid = false;
							}

							if (variant.toObject().contains("referenceCopyNumber"))
							{
								int reference_copy_number  = variant.toObject().value("referenceCopyNumber").toInt();

								if(reference_copy_number < 0)
								{
									errors << "Invalid entry '" + QString::number(reference_copy_number) + "' in 'referenceCopyNumber'!";
									is_valid = false;
								}
							}
							else
							{
								errors << "Required string 'referenceCopyNumber' in 'variant' missing!";
								is_valid = false;
							}
						}
						else if (type == VariantType::SVS)
						{
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
								int valid_int = chromosome_coordinates.value("start").toInt(-1);
								if (valid_int < 0)
								{
									errors << "Invalid entry '" + chromosome_coordinates.value("start").toString() + "' in 'start'!";
									is_valid = false;
								}
							}
							else
							{
								if (chromosome_coordinates.contains("innerStart"))
								{
									int valid_int = chromosome_coordinates.value("innerStart").toInt(-1);
									if (valid_int < 0)
									{
										errors << "Invalid entry '" + chromosome_coordinates.value("innerStart").toString() + "' in 'innerStart'!";
										is_valid = false;
									}
								}
								else
								{
									errors << "Required number 'innerStart' or 'start' in 'chromosomeCoordinates' missing!";
									is_valid = false;
								}

								if (chromosome_coordinates.contains("outerStart"))
								{
									int valid_int = chromosome_coordinates.value("outerStart").toInt(-1);
									if (valid_int < 0)
									{
										errors << "Invalid entry '" + chromosome_coordinates.value("outerStart").toString() + "' in 'outerStart'!";
										is_valid = false;
									}
								}
								else
								{
									errors << "Required number 'outerStart' or 'start' in 'chromosomeCoordinates' missing!";
									is_valid = false;
								}
							}

							if (chromosome_coordinates.contains("stop"))
							{
								int valid_int = chromosome_coordinates.value("stop").toInt(-1);
								if (valid_int < 0)
								{
									errors << "Invalid entry '" + chromosome_coordinates.value("stop").toString() + "' in 'stp'!";
									is_valid = false;
								}
							}
							else
							{
								if (chromosome_coordinates.contains("innerStop"))
								{
									int valid_int = chromosome_coordinates.value("innerStop").toInt(-1);
									if (valid_int < 0)
									{
										errors << "Invalid entry '" + chromosome_coordinates.value("innerStop").toString() + "' in 'innerStop'!";
										is_valid = false;
									}
								}
								else
								{
									errors << "Required number 'innerStop' or 'stop' in 'chromosomeCoordinates' missing!";
									is_valid = false;
								}

								if (chromosome_coordinates.contains("outerStop"))
								{
									int valid_int = chromosome_coordinates.value("outerStop").toInt(-1);
									if (valid_int < 0)
									{
										errors << "Invalid entry '" + chromosome_coordinates.value("outerStop").toString() + "' in 'outerStop'!";
										is_valid = false;
									}
								}
								else
								{
									errors << "Required number 'outerStop' or 'stop' in 'chromosomeCoordinates' missing!";
									is_valid = false;
								}
							}


						}
					}
                    else
                    {
                        errors << "Required JSON object 'chromosomeCoordinates' in 'variant' missing!";
                        is_valid = false;
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
	else if (clinvar_submission.contains("compoundHeterozygoteSet"))
	{
		//parse compoundHeterozygoteSet
		QJsonObject compound_heterozygote_set = clinvar_submission.value("compoundHeterozygoteSet").toObject();
		if (compound_heterozygote_set.contains("variantSets"))
		{
			QJsonArray variant_sets = compound_heterozygote_set.value("variantSets").toArray();
			if (variant_sets.size() == 2)
			{
				foreach (QJsonValue variant_set_buffer, variant_sets)
				{
					QJsonObject variant_set_item = variant_set_buffer.toObject();
					if (variant_set_item.contains("variantSet"))
					{
						QJsonObject variant_set = variant_set_item.value("variantSet").toObject();
						//parse variantSet
						if (variant_set.contains("variant"))
						{
							QJsonArray variant_array = variant_set.value("variant").toArray();
							if (variant_array.size() > 0)
							{
								foreach (const QJsonValue& variant, variant_array)
								{
									VariantType type = VariantType::SNVS_INDELS;

									if (variant.toObject().contains("variantType"))
									{
										QString variant_type = variant.toObject().value("variantType").toString();
										if (!VARIANT_TYPE.contains(variant_type))
										{
											errors << "Invalid variantType '" + variant_type + "'!";
											is_valid = false;
										}
										QStringList cnv_types;
										cnv_types << "copy number loss" << "copy number gain";
										QStringList sv_types;
										sv_types << "Insertion" << "Deletion" << "Duplication" << "Tandem duplication" << "Inversion" << "Translocation";
										if(cnv_types.contains(variant_type))
										{
											type = VariantType::CNVS;
										}
										else if(sv_types.contains(variant_type))
										{
											type = VariantType::SVS;
										}
										else
										{
											errors << "Invalid variantType '" + variant_type + "'!";
											is_valid = false;
										}
									}

									if (variant.toObject().contains("chromosomeCoordinates"))
									{
										QJsonObject chromosome_coordinates = variant.toObject().value("chromosomeCoordinates").toObject();

										if (type == VariantType::SNVS_INDELS)
										{
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
												int valid_int = chromosome_coordinates.value("start").toInt(-1);
												if (valid_int < 0)
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
												int valid_int = chromosome_coordinates.value("stop").toInt(-1);
												if (valid_int < 0)
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
										else if (type == VariantType::CNVS)
										{
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
												int valid_int = chromosome_coordinates.value("start").toInt(-1);
												if (valid_int < 0)
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
												int valid_int = chromosome_coordinates.value("stop").toInt(-1);
												if (valid_int < 0)
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

											if (variant.toObject().contains("copyNumber"))
											{
												QString copy_number_string = variant.toObject().value("copyNumber").toString().trimmed();
												bool ok = false;
												int copy_number = copy_number_string.toInt(&ok);

												if(!ok || copy_number < 0)
												{
													errors << "Invalid entry '" + copy_number_string + "' in 'copyNumber'!";
													is_valid = false;
												}
											}
											else
											{
												errors << "Required string 'copyNumber' in 'variant' missing!";
												is_valid = false;
											}

											if (variant.toObject().contains("referenceCopyNumber"))
											{
												int reference_copy_number = variant.toObject().value("referenceCopyNumber").toInt();

												if(reference_copy_number < 0)
												{
													errors << "Invalid entry '" + QString::number(reference_copy_number) + "' in 'referenceCopyNumber'!";
													is_valid = false;
												}
											}
											else
											{
												errors << "Required int 'referenceCopyNumber' in 'variant' missing!";
												is_valid = false;
											}
										}
										else if (type == VariantType::SVS)
										{
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
												int valid_int = chromosome_coordinates.value("start").toInt(-1);
												if (valid_int < 0)
												{
													errors << "Invalid entry '" + chromosome_coordinates.value("start").toString() + "' in 'start'!";
													is_valid = false;
												}
											}
											else
											{
												if (chromosome_coordinates.contains("innerStart"))
												{
													int valid_int = chromosome_coordinates.value("innerStart").toInt(-1);
													if (valid_int < 0)
													{
														errors << "Invalid entry '" + chromosome_coordinates.value("innerStart").toString() + "' in 'innerStart'!";
														is_valid = false;
													}
												}
												else
												{
													errors << "Required number 'innerStart' or 'start' in 'chromosomeCoordinates' missing!";
													is_valid = false;
												}

												if (chromosome_coordinates.contains("outerStart"))
												{
													int valid_int = chromosome_coordinates.value("outerStart").toInt(-1);
													if (valid_int < 0)
													{
														errors << "Invalid entry '" + chromosome_coordinates.value("outerStart").toString() + "' in 'outerStart'!";
														is_valid = false;
													}
												}
												else
												{
													errors << "Required number 'outerStart' or 'start' in 'chromosomeCoordinates' missing!";
													is_valid = false;
												}
											}

											if (chromosome_coordinates.contains("stop"))
											{
												int valid_int = chromosome_coordinates.value("stop").toInt(-1);
												if (valid_int < 0)
												{
													errors << "Invalid entry '" + chromosome_coordinates.value("stop").toString() + "' in 'stp'!";
													is_valid = false;
												}
											}
											else
											{
												if (chromosome_coordinates.contains("innerStop"))
												{
													int valid_int = chromosome_coordinates.value("innerStop").toInt(-1);
													if (valid_int < 0)
													{
														errors << "Invalid entry '" + chromosome_coordinates.value("innerStop").toString() + "' in 'innerStop'!";
														is_valid = false;
													}
												}
												else
												{
													errors << "Required number 'innerStop' or 'stop' in 'chromosomeCoordinates' missing!";
													is_valid = false;
												}

												if (chromosome_coordinates.contains("outerStop"))
												{
													int valid_int = chromosome_coordinates.value("outerStop").toInt(-1);
													if (valid_int < 0)
													{
														errors << "Invalid entry '" + chromosome_coordinates.value("outerStop").toString() + "' in 'outerStop'!";
														is_valid = false;
													}
												}
												else
												{
													errors << "Required number 'outerStop' or 'stop' in 'chromosomeCoordinates' missing!";
													is_valid = false;
												}
											}
										}
									}
									else
									{
										errors << "Required JSON object 'chromosomeCoordinates' in 'variant' missing!";
										is_valid = false;
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
						errors << "Required JSON array 'variantSet' in 'variantSets' missing!";
						is_valid = false;
					}

				}

			}
			else
			{
				errors << "Array 'variantSets' has to contain exactly 2 elements of type 'variantSetType'!";
				is_valid = false;
			}
		}
		else
		{
			errors << "Required array 'variantSets' in 'compoundHeterozygoteSet' missing!";
			is_valid = false;
		}

	}
	else
    {
		errors << "Required string 'variantSet' or 'compoundHeterozygoteSet' in 'clinvarSubmission' missing!";
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

	if (json.contains("clinvarSubmissionReleaseStatus"))
	{
		QStringList release_status = QStringList() <<  "public" << "hold until published";
		if (!release_status.contains(json.value("clinvarSubmissionReleaseStatus").toString()))
		{
			errors << "Invalid entry '" + json.value("clinvarSubmissionReleaseStatus").toString() + "' in 'clinvarSubmissionReleaseStatus'!";
			is_valid = false;
		}
	}
	else
	{
		errors << "Required string 'clinvarSubmissionReleaseStatus' is missing!";
		is_valid = false;
	}

	return is_valid;
}

QString ClinvarUploadDialog::getHGVS()
{
	QString ref_file = Settings::string("reference_genome", true);
	if (ref_file=="") THROW(ArgumentException, "Reference genome required to create HGVS representation!");
	FastaFileIndex genome_index(ref_file);
	bool mito = false;
	QString hgvs_var1, hgvs_var2;
	if(clinvar_upload_data_.variant_type1 == VariantType::SNVS_INDELS)
	{
		Chromosome chr = Chromosome(ui_.cb_chr_snv1->currentText());
		Variant variant1 = Variant(chr, ui_.le_start_snv1->text().toInt(), ui_.le_end_snv1->text().toInt(), ui_.le_ref_snv1->text().toUtf8(), ui_.le_obs_snv1->text().toUtf8());
		variant1.normalize("-", true);
		hgvs_var1 = variant1.toHGVS(genome_index).remove(0, 2);
		mito = chr.isM();
	}
	else if(clinvar_upload_data_.variant_type1 == VariantType::CNVS)
	{
		Chromosome chr = Chromosome(ui_.cb_chr_cnv1->currentText());
		hgvs_var1 = ui_.le_start_cnv1->text() + "_" + ui_.le_end_cnv1->text() + ((ui_.le_cn_cnv1->text().toInt() < ui_.le_rcn_cnv1->text().toInt())?"del":"dup");
		mito = chr.isM();
	}
	else if(clinvar_upload_data_.variant_type1 == VariantType::SVS)
	{
		bool ok;
		StructuralVariantType type = StructuralVariantTypeFromString(ui_.cb_type_sv1->currentText());
		BedpeLine sv = BedpeLine(Chromosome(ui_.cb_chr1_sv1->currentText()), ui_.le_start1_sv1->text().toInt(&ok), ui_.le_end2_sv1->text().toInt(&ok),
								 Chromosome(ui_.cb_chr2_sv1->currentText()), ui_.le_start2_sv1->text().toInt(&ok), ui_.le_end2_sv1->text().toInt(&ok), type, QByteArrayList());
		if(!ok) THROW(ArgumentException, "Error during number conversion!");
		BedLine range = sv.affectedRegion()[0];
		switch (type)
		{
			case StructuralVariantType::DEL:
			case StructuralVariantType::DUP:
			case StructuralVariantType::INV:
				hgvs_var1 = QString::number(range.start()) + "_" + QString::number(range.end()) + StructuralVariantTypeToString(type).toLower();
				mito = range.chr().isM();
				break;

			case StructuralVariantType::INS:
				hgvs_var1 = QString::number(range.start()) + "_" + QString::number(range.end()) + "insN[?]";
				break;

			case StructuralVariantType::BND:
				THROW(NotImplementedException, "Submission of translocations are currently not supported!");
				break;

			default:
				break;
		}
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type provided for variant 1!")
	}

	if(clinvar_upload_data_.variant_type2 == VariantType::SNVS_INDELS)
	{
		Chromosome chr = Chromosome(ui_.cb_chr_snv2->currentText());
		Variant variant2 = Variant(chr, ui_.le_start_snv2->text().toInt(), ui_.le_end_snv2->text().toInt(), ui_.le_ref_snv2->text().toUtf8(), ui_.le_obs_snv2->text().toUtf8());
		variant2.normalize("-", true);
		hgvs_var2 = variant2.toHGVS(genome_index).remove(0, 2);
	}
	else if(clinvar_upload_data_.variant_type2 == VariantType::CNVS)
	{
		Chromosome chr = Chromosome(ui_.cb_chr_cnv2->currentText());
		hgvs_var2 = ui_.le_start_cnv2->text() + "_" + ui_.le_end_cnv2->text() + ((ui_.le_cn_cnv2->text().toInt() < ui_.le_rcn_cnv2->text().toInt())?"del":"dup");
	}
	else if(clinvar_upload_data_.variant_type2 == VariantType::SVS)
	{
		StructuralVariantType type = StructuralVariantTypeFromString(ui_.cb_type_sv2->currentText());
		bool ok;
		BedpeLine sv = BedpeLine(Chromosome(ui_.cb_chr1_sv2->currentText()), ui_.le_start1_sv2->text().toInt(&ok), ui_.le_end2_sv2->text().toInt(&ok),
								 Chromosome(ui_.cb_chr2_sv2->currentText()), ui_.le_start2_sv2->text().toInt(&ok), ui_.le_end2_sv2->text().toInt(&ok), type, QByteArrayList());
		if(!ok) THROW(ArgumentException, "Error during number conversion!");
		BedLine range = sv.affectedRegion()[0];
		switch (type)
		{
			case StructuralVariantType::DEL:
			case StructuralVariantType::DUP:
			case StructuralVariantType::INV:
				hgvs_var2 = QString::number(range.start()) + "_" + QString::number(range.end()) + StructuralVariantTypeToString(type).toLower();
				mito = range.chr().isM();
				break;

			case StructuralVariantType::INS:
				hgvs_var2 = QString::number(range.start()) + "_" + QString::number(range.end()) + "insN[?]";
				break;
			case StructuralVariantType::BND:
				THROW(NotImplementedException, "Submission of translocations are currently not supported!");
				break;
			default:
				break;
		}
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type provided for variant 2!")
	}

	return QString((mito)?"m":"g") + "[" + hgvs_var1 + "];[" + hgvs_var2 + "]";
}

QString ClinvarUploadDialog::getDbTableName(bool var2)
{
	if(var2)
	{
		if(clinvar_upload_data_.variant_type2 == VariantType::SNVS_INDELS)
		{
			return "variant";
		}
		if(clinvar_upload_data_.variant_type2 == VariantType::CNVS)
		{
			return "cnv";
		}
		if(clinvar_upload_data_.variant_type2 == VariantType::SVS)
		{
			return db_.svTableName(clinvar_upload_data_.sv2.type());
		}
	}
	else
	{
		if(clinvar_upload_data_.variant_type1 == VariantType::SNVS_INDELS)
		{
			return "variant";
		}
		if(clinvar_upload_data_.variant_type1 == VariantType::CNVS)
		{
			return "cnv";
		}
		if(clinvar_upload_data_.variant_type1 == VariantType::SVS)
		{
			return db_.svTableName(clinvar_upload_data_.sv1.type());
		}
	}
	THROW(ArgumentException, "Invalid variant type!");
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

QString ClinvarUploadDialog::convertClassification(QString classification, bool reverse)
{
	if (reverse)
	{
		if (classification=="Pathogenic")
		{
			return "5";
		}
		if (classification=="Likely pathogenic")
		{
			return "4";
		}
		if (classification=="Uncertain significance")
		{
			return "3";
		}
		if (classification=="Likely benign")
		{
			return "2";
		}
		if (classification=="Benign")
		{
			return "1";
		}
//		if (classification=="Established risk allele")
//		{
//			return "M";
//		}
//		if (classification=="not provided")
//		{
//			return "n/a";
//		}
	}
	else
	{
		if (classification=="5")
		{
			return "Pathogenic";
		}
		if (classification=="4")
		{
			return "Likely pathogenic";
		}
		if (classification=="3" || classification=="")
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
		if (classification=="M")
		{
			return "Established risk allele";
		}
		if (classification=="n/a")
		{
			return "not provided";
		}
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
//	"Pathogenic, low penetrance",
//	"Uncertain risk allele",
//	"Likely pathogenic, low penetrance",
//	"Established risk allele",
//	"Likely risk allele",
//	"affects",
//	"association",
//	"drug response",
//	"confers sensitivity",
//	"protective",
//	"other",
//	"not provided"
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
