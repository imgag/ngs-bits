#include "ClinvarUploadStatusWidget.h"
#include "ui_ClinvarUploadStatusWidget.h"

#include "GUIHelper.h"
#include "HttpHandler.h"
#include "LoginManager.h"
#include "Settings.h"

#include <QJsonDocument>

ClinvarUploadStatusWidget::ClinvarUploadStatusWidget(QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::ClinvarUploadStatusWidget),
	http_handler_(HttpRequestHandler::INI, this)
{
	// abort if no connection to NGSD
	if (!LoginManager::active())
	{
		GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to design cfDNA panels!");
		this->close();
	}

	ui_->setupUi(this);

	fillTable();
}

ClinvarUploadStatusWidget::~ClinvarUploadStatusWidget()
{
	delete ui_;
}

void ClinvarUploadStatusWidget::fillTable()
{
	//Test
	static HttpHandler http_handler(HttpRequestHandler::INI); //static to allow caching of credentials

	int col_count = 9;
	// init header
	int col_idx = 0;
	ui_->clinvar_table->setColumnCount(col_count);

	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("sample"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("variant"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("class"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("user"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("date"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("submission id"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("status"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("stable id"));
	ui_->clinvar_table->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("comment"));


	// get publication table
	SqlQuery query = db_.getQuery();
	query.exec("SELECT sample_id, variant_id, class, details, user_id, date FROM variant_publication WHERE db='ClinVar' ORDER BY date ASC;");

	// fill table
	int row_count = query.size();
	ui_->clinvar_table->setRowCount(row_count);
	int row_idx = 0;

	while(query.next())
	{
		col_idx = 0;
		QString details = query.value("details").toString();

		// skip publications without submission id
		if (!details.contains("submission_id=SUB")) continue;

		QString sample_name = db_.getSampleData(query.value("sample_id").toString()).name;
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(sample_name));
		QString variant_string = db_.variant(query.value("variant_id").toString()).toString();
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant_string));
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(query.value("class").toString()));
		QString user = db_.userName(query.value("user_id").toInt());
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(user));
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(query.value("date").toDateTime().toString("dd.MM.yyyy hh:mm")));
		QString submission_id;
		foreach (const QString& key_value_pair, details.split(';'))
		{
			if (key_value_pair.startsWith("submission_id="))
			{
				submission_id = key_value_pair.split("=").at(1).trimmed();
				break;
			}
		}
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(submission_id));

		// get status
		QString status = getSubmissionStatus(submission_id, http_handler);
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(status));
		QString stable_id;
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(stable_id));
		QString comment;
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(comment));
		row_idx++;

		//debug
		if (row_idx > 1) break;
	}

	ui_->clinvar_table->setRowCount(row_idx);


	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->clinvar_table, 250);



}

QString ClinvarUploadStatusWidget::getSubmissionStatus(const QString& submission_id, HttpHandler& http_handler)
{
	// read API key
	QByteArray api_key = Settings::string("clinvar_api_key").trimmed().toUtf8();
	if (api_key.isEmpty()) THROW(FileParseException, "Settings INI file does not contain ClinVar API key!");

	try
	{
		//add headers
		HttpHeaders add_headers;
		add_headers.insert("Content-Type", "application/json");
		add_headers.insert("SP-API-KEY", api_key);

		//get request
		QByteArray reply = http_handler.get("https://submit.ncbi.nlm.nih.gov/api/v1/submissions/" + submission_id.toUpper() + "/actions/", add_headers);


		qDebug() << reply;
		// parse response
		QJsonObject response = QJsonDocument::fromJson(reply).array().at(0);

		//extract status
		QJsonValue test = response.value("actions");
		qDebug() << test;
		/*.at(0).value("status")*/

		return "" ;



	}
	catch(Exception e)
	{
		qDebug() << "Status check failed for submission " << submission_id << " (" << e.message() << ")!";

		return "";
	}


//    QJsonObject post_request;
//    QJsonArray actions;
//    QJsonObject action;
//    action.insert("type", "AddData");
//    action.insert("targetDb", "clinvar");
//    QJsonObject data;
//    data.insert("content", clinvar_submission);
//    action.insert("data", data);
//    actions.append(action);
//    post_request.insert("actions", actions);


//    // create http request and parse result
//    static HttpHandler http_handler(HttpRequestHandler::INI); //static to allow caching of credentials
//    try
//    {
//        QStringList messages;
//        messages << ui_.comment_upload->toPlainText();





//        // parse response
//        bool success = false;
//        QString submission_id;
//        QJsonObject response = QJsonDocument::fromJson(reply).object();

//        //successful dry-run
//        if (response.isEmpty())
//        {
//            messages << "MESSAGE: Dry-run successful!";
////			QFile jsonFile(clinvar_upload_data_.processed_sample + "_submission_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".json");
////			jsonFile.open(QFile::WriteOnly);
////			jsonFile.write(QJsonDocument(clinvar_submission).toJson());
//        }
//        else if (response.contains("id"))
//        {
//            //successfully submitted
//            messages << "MESSAGE: The submission was successful!";
//            submission_id = response.value("id").toString();
//            messages << "MESSAGE: Submission ID: " + submission_id;
//            success = true;
//        }
//        else if (response.contains("message"))
//        {
//            //errors
//            messages << "ERROR: " + response.value("message").toString();
//        }

//        if (success)
//        {
//            // add entry to the NGSD
//            QStringList details;
//            details << "submission_id=" + submission_id;
//            //clinical significance
//            details << "clinical_significance_desc=" + ui_.cb_clin_sig_desc->currentText();
//            details << "clinical_significance_comment=" + VcfFile::encodeInfoValue(ui_.le_clin_sig_desc_comment->text());
//            details << "last_evaluatued=" + ui_.de_last_eval->date().toString("yyyy-MM-dd");
//            details << "mode_of_inheritance=" + ui_.cb_inheritance->currentText();
//            //condition set
//            QStringList condition;
//            for (int row_idx = 0; row_idx < ui_.tw_disease_info->rowCount(); ++row_idx)
//            {
//                condition << ui_.tw_disease_info->item(row_idx, 0)->text() + "|" + ui_.tw_disease_info->item(row_idx, 1)->text();
//            }
//            details << "condition=" + condition.join(',');
//			details << "variant_id=" + QString::number(clinvar_upload_data_.variant_id);
//			details << "variant_rc_id=" + QString::number(clinvar_upload_data_.variant_report_config_id);
//            //observed in
//            details << "affected_status=" + ui_.cb_affected_status->currentText();
//            details << "allele_origin=" + ui_.cb_allele_origin->currentText();
//            QStringList phenotypes;
//            foreach (const Phenotype& phenotype, ui_.phenos->selectedPhenotypes())
//            {
//                phenotypes << phenotype.accession();
//            }
//            details << "clinical_features=" + phenotypes.join(',');
//            details << "clinical_feature_comment=" + VcfFile::encodeInfoValue(ui_.le_clin_feat_comment->text());
//			details << "collection_method=" + ui_.cb_collection_method->currentText();

//            details << "release_status=" + ui_.cb_release_status->currentText();
//            details << "gene=" +  NGSD().genesToApproved(GeneSet::createFromStringList(ui_.le_gene->text().replace(";", ",").split(','))).toStringList().join(',');

//            // log publication in NGSD
//            db_.addVariantPublication(clinvar_upload_data_.processed_sample, clinvar_upload_data_.variant, "ClinVar", clinvar_upload_data_.report_variant_config.classification, details.join(";"));

//            //show result
//            QStringList lines;
//            lines << "DATA UPLOAD TO CLINVAR SUCCESSFUL";
//            lines << "";
//            lines << messages.join("\n");
//            lines << "";
//            lines << "sample: " + clinvar_upload_data_.processed_sample;
//            lines << "user: " + LoginManager::user();
//            lines << "date: " + Helper::dateTime();
//            lines << "";
//            lines << details;

//            ui_.comment_upload->setText(lines.join("\n").replace("=", ": "));

//            //write report file to transfer folder
//            QString gsvar_publication_folder = Settings::path("gsvar_publication_folder");
//            if (gsvar_publication_folder!="")
//            {
//					QString file_rep = gsvar_publication_folder + "/" + clinvar_upload_data_.processed_sample + "_CLINVAR_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
//                    Helper::storeTextFile(file_rep, ui_.comment_upload->toPlainText().split("\n"));
//            }

//        }
//        else
//        {
//            // Upload failed
//            ui_.comment_upload->setText("DATA UPLOAD ERROR:\n" + messages.join("\n"));
//            ui_.upload_btn->setEnabled(true);
//        }
//    }
//    catch(Exception e)
//    {
//        ui_.comment_upload->setText("DATA UPLOAD FAILED:\n" + e.message());
//        ui_.upload_btn->setEnabled(true);
//    }
}
