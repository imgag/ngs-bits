#include "ClinvarUploadStatusWidget.h"
#include "ui_ClinvarUploadStatusWidget.h"

#include "ClinvarUploadDialog.h"
#include "GUIHelper.h"
#include "HttpHandler.h"
#include "LoginManager.h"
#include "Settings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QMessageBox>

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


	// connect signals and slots
	connect(ui_->clinvar_table,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));

	// set context menus for table
	ui_->clinvar_table->setContextMenuPolicy(Qt::CustomContextMenu);

	fillTable();
}

ClinvarUploadStatusWidget::~ClinvarUploadStatusWidget()
{
	delete ui_;
}

void ClinvarUploadStatusWidget::fillTable()
{
	//init background colors
	// define table backround colors
	QColor bg_red = Qt::red;
	bg_red.setAlphaF(0.5);
	QColor bg_green = Qt::darkGreen;
	bg_green.setAlphaF(0.5);
	QColor bg_orange = QColor(255, 135, 60);
	bg_orange.setAlphaF(0.5);

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
	QString limit_to_user;
	if (LoginManager::role() != "admin")
	{
		limit_to_user = " AND user_id=" + QString::number(LoginManager::userId());
	}
	query.exec("SELECT id, sample_id, variant_id, class, details, user_id, date, result FROM variant_publication WHERE db='ClinVar'" + limit_to_user + " ORDER BY date DESC;");

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

		//store id in vertical header item
		int var_pub_id = query.value("id").toInt();
		QTableWidgetItem* vertical_header_item = new QTableWidgetItem();
		vertical_header_item->setData(Qt::UserRole, var_pub_id);
		ui_->clinvar_table->setVerticalHeaderItem(row_idx, vertical_header_item);

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

		SubmissionStatus submission_status;
		QString result = query.value("result").toString();
		// parse result string
		submission_status.status = result.split(';').at(0);

		if (submission_status.status == "processed")
		{
			submission_status.stable_id = result.split(';').at(1);
		}
		else if (submission_status.status == "error")
		{
			submission_status.comment = result.split(';').at(1);
		}
		else
		{
			// get status from clinVar
			submission_status = getSubmissionStatus(submission_id, http_handler);

			if (submission_status.status == "processed" || submission_status.status == "error")
			{
				QString result = submission_status.status + ";";
				if (submission_status.status == "processed")
				{
					result += submission_status.stable_id;
				}
				else //submission_status.status == "error"
				{
					result += submission_status.comment;
				}
				//update result info in the NGSD
				db_.updateVariantPublicationResult(query.value("id").toInt(), result);
			}
		}

		QTableWidgetItem* table_cell = GUIHelper::createTableItem(submission_status.status);
		if (submission_status.status == "processed")
		{
			table_cell->setBackgroundColor(bg_green);
		}
		else if(submission_status.status == "error")
		{
			table_cell->setBackgroundColor(bg_red);
		}

		ui_->clinvar_table->setItem(row_idx, col_idx++, table_cell);
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(submission_status.stable_id));
		ui_->clinvar_table->setItem(row_idx, col_idx++, GUIHelper::createTableItem(submission_status.comment));

		row_idx++;

	}

	ui_->clinvar_table->setRowCount(row_idx);


	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->clinvar_table, 250, false);

}

SubmissionStatus ClinvarUploadStatusWidget::getSubmissionStatus(const QString& submission_id, HttpHandler& http_handler)
{
	// read API key
	QByteArray api_key = Settings::string("clinvar_api_key").trimmed().toUtf8();
	if (api_key.isEmpty()) THROW(FileParseException, "Settings INI file does not contain ClinVar API key!");

	SubmissionStatus submission_status;

	try
	{
		//add headers
		HttpHeaders add_headers;
		add_headers.insert("Content-Type", "application/json");
		add_headers.insert("SP-API-KEY", api_key);

		//get request
		QByteArray reply = http_handler.get("https://submit.ncbi.nlm.nih.gov/api/v1/submissions/" + submission_id.toUpper() + "/actions/", add_headers);

		// parse response
		QJsonObject response = QJsonDocument::fromJson(reply).object();

		//extract status
		QJsonArray actions = response.value("actions").toArray();
		submission_status.status = actions.at(0).toObject().value("status").toString();

		if (submission_status.status == "processed" || submission_status.status == "error")
		{
			//get summary file and extract stable id or error message
			QString report_summary_file = actions.at(0).toObject().value("responses").toArray().at(0).toObject().value("files").toArray().at(0).toObject().value("url").toString();
			QByteArray summary_reply = http_handler.get(report_summary_file);
			QJsonDocument summary_response = QJsonDocument::fromJson(summary_reply);

			if (submission_status.status == "processed")
			{
				// get stable id
				submission_status.stable_id = summary_response.object().value("submissions").toArray().at(0).toObject().value("identifiers").toObject().value("clinvarAccession").toString();
			}
			if (submission_status.status == "error")
			{
				// get error message
				QJsonArray errors = summary_response.object().value("submissions").toArray().at(0).toObject().value("errors").toArray();
				QStringList error_messages;
				foreach (const QJsonValue& error, errors)
				{
					error_messages << error.toObject().value("output").toObject().value("errors").toArray().at(0).toObject().value("userMessage").toString();
				}
				submission_status.comment = error_messages.join("\n");
			}


		}

		return submission_status;



	}
	catch(Exception e)
	{
		qDebug() << "Status check failed for submission " << submission_id << " (" << e.message() << ")!";

		return SubmissionStatus();
	}

}

ClinvarUploadData ClinvarUploadStatusWidget::getClinvarUploadData(int var_pub_id)
{
	ClinvarUploadData data;

	data.variant_publication_id = var_pub_id;

	// get publication data
	SqlQuery query = db_.getQuery();
	query.prepare("SELECT sample_id, variant_id, details, user_id FROM variant_publication WHERE id=:0");
	query.bindValue(0, var_pub_id);
	query.exec();

	if (query.size() != 1) THROW(DatabaseException, "Invalid variant publication id!");
	query.next();

	QString sample_id = query.value("sample_id").toString();
	data.variant_id = query.value("variant_id").toInt();
	QStringList details = query.value("details").toString().split(';');
	data.user_id = query.value("user_id").toInt();


	//get sample data
	SampleData sample_data = db_.getSampleData(sample_id);

	//get disease info
	data.disease_info = db_.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
	data.disease_info.append(db_.getSampleDiseaseInfo(sample_id, "Orpha number"));
	if (data.disease_info.length() < 1)
	{
		QMessageBox::warning(this, "No disease info", "The sample has to have at least one OMIM or Orphanet disease identifier to publish a variant in ClinVar.");
		return data;
	}

	// get affected status
	data.affected_status = sample_data.disease_status;

	//get phenotype(s)
	data.phenos = sample_data.phenotypes;

	//get variant info
	data.variant = db_.variant(QString::number(data.variant_id));

	//get variant report config id
	data.report_config_variant_id = -1;
	foreach (const QString& kv_pair, details)
	{
		if (kv_pair.startsWith("variant_rc_id="))
		{
			data.report_config_variant_id = Helper::toInt(kv_pair.split('=').at(1), "variant_rc_id");
			qDebug() << "Report configuration variant id: " << data.report_config_variant_id;
			break;
		}
	}
	if (data.report_config_variant_id < 0)
	{
		THROW(DatabaseException, "No report variant config id information found in variant publication!");
	}

	//get variant report config
	query.exec("SELECT * FROM report_configuration_variant WHERE id=" + QString::number(data.report_config_variant_id));
	if (query.size() != 1) THROW(DatabaseException, "Invalid report config variant id!");
	query.next();
	data.report_variant_config.variant_index = -1;
	data.report_variant_config.report_type = query.value("type").toString();
	data.report_variant_config.causal = query.value("causal").toBool();
	data.report_variant_config.inheritance = query.value("inheritance").toString();
	data.report_variant_config.de_novo = query.value("de_novo").toBool();
	data.report_variant_config.mosaic = query.value("mosaic").toBool();
	data.report_variant_config.comp_het = query.value("compound_heterozygous").toBool();
	data.report_variant_config.exclude_artefact = query.value("exclude_artefact").toBool();
	data.report_variant_config.exclude_frequency = query.value("exclude_frequency").toBool();
	data.report_variant_config.exclude_phenotype = query.value("exclude_phenotype").toBool();
	data.report_variant_config.exclude_mechanism = query.value("exclude_mechanism").toBool();
	data.report_variant_config.exclude_other = query.value("exclude_other").toBool();
	data.report_variant_config.comments = query.value("comments").toString();
	data.report_variant_config.comments2 = query.value("comments2").toString();

	//get classification
	data.report_variant_config.classification = db_.getClassification(data.variant).classification;
	if (data.report_variant_config.classification.trimmed().isEmpty() || (data.report_variant_config.classification.trimmed() == "n/a"))
	{
		QMessageBox::warning(this, "No Classification", "The variant has to have a classification to be published!");
		return data;
	}

	// get report config
	int rc_id = db_.getValue("SELECT report_configuration_id FROM report_configuration_variant WHERE id=" + QString::number(data.report_config_variant_id) + " AND variant_id="
							+ QString::number(data.variant_id), false).toInt();

	//get genes
	data.genes = db_.genesOverlapping(data.variant.chr(), data.variant.start(), data.variant.end(), 5000);

	//get processed sample id
	data.processed_sample = db_.processedSampleName(db_.getValue("SELECT processed_sample_id FROM report_configuration WHERE id=" + QString::number(rc_id)).toString());


	return data;
}

void ClinvarUploadStatusWidget::showContextMenu(QPoint pos)
{
	int row_idx = ui_->clinvar_table->itemAt(pos)->row();

	//create context menu based on PRS entry
	QMenu menu(ui_->clinvar_table);

	QAction* resubmit = menu.addAction("Resubmit variant publication");
	resubmit->setEnabled(ui_->clinvar_table->item(row_idx, 6)->text() == "error");
	QAction* edit = menu.addAction("Edit variant publication");
	edit->setEnabled(ui_->clinvar_table->item(row_idx, 6)->text() == "processed");

	//execute menu
	QAction* action = menu.exec(ui_->clinvar_table->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;

	if (action == resubmit || action == edit)
	{
		// get publication id
		int var_pub_id = ui_->clinvar_table->verticalHeaderItem(row_idx)->data(Qt::UserRole).toInt();

		qDebug() << "Publication id:" << var_pub_id;

		// get ClinVar upload data
		ClinvarUploadData data = getClinvarUploadData(var_pub_id);

		if (data.processed_sample.isEmpty())
		{
			QMessageBox::warning(this, "Upload data incomplete", "The ClinVar upload data is incomplete. Cannot perform reupload.");
			return;
		}

		//add stable id
		if (action == edit)
		{
			data.stable_id = ui_->clinvar_table->item(row_idx, 7)->text();
		}


		// show dialog
		ClinvarUploadDialog dlg(this);
		dlg.setData(data);
		dlg.exec();

	}
}
