#include <QAction>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>
#include <QMessageBox>
#include "PublishedVariantsWidget.h"
#include "ui_PublishedVariantsWidget.h"
#include "NGSD.h"
#include "NGSHelper.h"
#include "GSvarHelper.h"
#include <QAction>

PublishedVariantsWidget::PublishedVariantsWidget(QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::PublishedVariantsWidget)
	, http_handler_(HttpRequestHandler::INI, this)
{
	ui_->setupUi(this);
	connect(ui_->search_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));

	//fill filter boxes
	NGSD db;
	ui_->f_sample->fill(db.createTable("sample", "SELECT id, name FROM sample"));
	ui_->f_published->fill(db.createTable("user", "SELECT id, name FROM user ORDER BY name ASC"));
	ui_->f_db->addItem("n/a");
	ui_->f_db->addItems(db.getEnum("variant_publication", "db"));

	//link to LOVD
	QAction* action = new QAction(QIcon(":/Icons/LOVD.png"), "Find in LOVD", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(searchForVariantInLOVD()));

	//link to ClinVar
	action = new QAction(QIcon(":/Icons/ClinGen.png"), "Find in ClinVar", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(searchForVariantInClinVar()));
}

PublishedVariantsWidget::~PublishedVariantsWidget()
{
	delete ui_;
}

void PublishedVariantsWidget::updateTable()
{
	//init
	NGSD db;
	QStringList constraints;

	//filter "published by"
	if (ui_->f_published->isValidSelection())
	{
		constraints << "user_id='" + ui_->f_published->getId() + "'";
	}
	ui_->f_published->showVisuallyIfValid(true);

	//filter "region"
	try
	{
		if (!ui_->f_region->text().trimmed().isEmpty())
		{
			Chromosome chr;
			int start, end;
			NGSHelper::parseRegion(ui_->f_region->text(), chr, start, end);
			constraints << ("variant_id IN (SELECT id FROM variant where chr='" + chr.strNormalized(true) + "' AND start>=" + QString::number(start) + " AND end<=" + QString::number(end) + ")");

			ui_->f_region->setStyleSheet("");
		}
	}
	catch (...)
	{
		ui_->f_region->setStyleSheet("QLineEdit {border: 2px solid red;}");
	}

	//filter "DB"
	if (ui_->f_db->currentText()!="n/a")
	{
		constraints << ("db='" + ui_->f_db->currentText() + "'");
	}

	//filter "sample"
	if (ui_->f_sample->isValidSelection())
	{
		constraints << "sample_id='" + ui_->f_sample->getId() + "'";
	}
	ui_->f_sample->showVisuallyIfValid(true);

	//filter "replaced"
	if (!ui_->cb_show_replaced->isChecked())
	{
		constraints << "replaced=FALSE";
	}


	//create table
	QApplication::setOverrideCursor(Qt::BusyCursor);
	QString constraints_str;
	if (!constraints.isEmpty())
	{
		constraints_str = " WHERE (" + constraints.join(") AND (") + ")";
	}
	DBTable table = db.createTable("variant_publication", "SELECT * FROM variant_publication" + constraints_str + " ORDER BY id ASC");

	//replace foreign keys
	db.replaceForeignKeyColumn(table, table.columnIndex("sample_id"), "sample", "name");
	db.replaceForeignKeyColumn(table, table.columnIndex("user_id"), "user", "name");
	db.replaceForeignKeyColumn(table, table.columnIndex("variant_id"), "variant", "CONCAT(chr, ':', start, '-', end, ' ', ref, '>', obs)");

	//rename columns (after keys)
	QStringList headers = table.headers();
	headers.replace(headers.indexOf("sample_id"), "sample");
	headers.replace(headers.indexOf("user_id"), "published by");
	headers.replace(headers.indexOf("variant_id"), "variant");
	table.setHeaders(headers);

	//remove 'replaced' column
	table.takeColumn(table.columnIndex("replaced"));

	//remove 'result' column and split it into multiple columns
	int result_idx = table.columnIndex("result");
	int db_idx = table.columnIndex("db");
	int details_idx = table.columnIndex("details");
	int id_idx = table.columnIndex("id");
	QStringList results = table.takeColumn(result_idx);
	QStringList status, stable_ids, error_messages;
	int row_idx = 0;
	foreach (const QString& result, results)
	{
		if (result.isEmpty())
		{
			if ((table.row(row_idx).value(db_idx) == "ClinVar") && (table.row(row_idx).value(details_idx).contains("submission_id=SUB")))
			{
				//get submission id
				QString submission_id;
				foreach (const QString& key_value_pair, table.row(row_idx).value(details_idx).split(';'))
				{
					if (key_value_pair.startsWith("submission_id="))
					{
						submission_id = key_value_pair.split("=").at(1).trimmed();
						break;
					}
				}
				if (submission_id.isEmpty()) THROW(ArgumentException, "No ClinVar submission id found!");
				//get status from ClinVar
				SubmissionStatus submission_status = getSubmissionStatus(submission_id, http_handler_);
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
					db.updateVariantPublicationResult(table.row(row_idx).value(id_idx).toInt(), result);
				}
				status << "";
				stable_ids << "";
				error_messages << "";
			}
			else
			{
				status << "";
				stable_ids << "";
				error_messages << "";

			}

		}
		else if (result.startsWith("error;"))
		{
			status << "error";
			stable_ids << "";
			error_messages << result.split(";").at(1);
		}
		else if (result.startsWith("processed;"))
		{
			status << "processed";
			stable_ids << result.split(";").at(1);
			error_messages << "";
		}
		else
		{
			status << result;
			stable_ids << "";
			error_messages << "";
		}
		row_idx++;
	}
	table.addColumn(status, "submission status");
	table.addColumn(stable_ids, "ClinVar accession id");
	table.addColumn(error_messages, "errors");



	//filter by text
	QString text_filter = ui_->f_text->text().trimmed();
	if (text_filter!="")
	{
		table.filterRows(text_filter);
	}

	//show data
	ui_->table->setData(table);

	//color results
	ui_->table->setBackgroundColorIfEqual("submission status", Qt::darkRed, "error");
	ui_->table->setBackgroundColorIfEqual("submission status", Qt::darkGreen, "processed");
	QApplication::restoreOverrideCursor();
}

void PublishedVariantsWidget::searchForVariantInLOVD()
{
	try
	{
		int col = ui_->table->columnIndex("variant");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_text = ui_->table->item(row, col)->text();
			Variant variant = Variant::fromString(variant_text);

			int pos = variant.start();
			if (variant.ref()=="-") pos += 1;
			QDesktopServices::openUrl(QUrl("https://databases.lovd.nl/shared/variants#search_chromosome=" + variant.chr().strNormalized(false) + "&search_VariantOnGenome/DNA"+(GSvarHelper::build()==GenomeBuild::HG38 ? "/hg38" : "")+"=g." + QString::number(pos)));
		}
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "LOVD search error", e.message());
	}
}

void PublishedVariantsWidget::searchForVariantInClinVar()
{
	try
	{
		int col = ui_->table->columnIndex("variant");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_text = ui_->table->item(row, col)->text();
			Variant variant = Variant::fromString(variant_text);

			int start = variant.start();
			int end = variant.end();

			QDesktopServices::openUrl(QUrl("https://www.ncbi.nlm.nih.gov/clinvar/?term=" + variant.chr().strNormalized(false) + "[chr]+AND+" + QString::number(start) + ":" + QString::number(end) + (GSvarHelper::build()==GenomeBuild::HG38 ? "[cpos]" : "[chrpos37]")));
		}
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "LOVD search error", e.message());
	}
}

void PublishedVariantsWidget::showContextMenu(QPoint pos)
{
	int row_idx = ui_->table->itemAt(pos)->row();
	int status_idx = ui_->table->columnIndex("submission status");
	int accession_idx = ui_->table->columnIndex("ClinVar accession id");

	//create context menu based on PRS entry
	QMenu menu(ui_->table);

	QAction* resubmit = menu.addAction("Resubmit variant publication");
	resubmit->setEnabled(ui_->table->item(row_idx, status_idx)->text().startsWith("error;"));
	QAction* edit = menu.addAction("Edit variant publication");
	edit->setEnabled(ui_->table->item(row_idx, status_idx)->text().startsWith("processed;"));

	//execute menu
	QAction* action = menu.exec(ui_->table->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;

	if (action == resubmit || action == edit)
	{
		// get publication id
		int var_pub_id = ui_->table->getId(row_idx).toInt();

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

			data.stable_id = ui_->table->item(row_idx, accession_idx)->text();
		}


		// show dialog
		ClinvarUploadDialog dlg(this);
		dlg.setData(data);
		dlg.exec();

	}
}

SubmissionStatus PublishedVariantsWidget::getSubmissionStatus(const QString& submission_id, HttpHandler& http_handler)
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

ClinvarUploadData PublishedVariantsWidget::getClinvarUploadData(int var_pub_id)
{
	ClinvarUploadData data;
	NGSD db;

	data.variant_publication_id = var_pub_id;

	// get publication data
	SqlQuery query = db.getQuery();
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
	SampleData sample_data = db.getSampleData(sample_id);

	//get disease info
	data.disease_info = db.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
	data.disease_info.append(db.getSampleDiseaseInfo(sample_id, "Orpha number"));
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
	data.variant = db.variant(QString::number(data.variant_id));

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
	data.report_variant_config.classification = db.getClassification(data.variant).classification;
	if (data.report_variant_config.classification.trimmed().isEmpty() || (data.report_variant_config.classification.trimmed() == "n/a"))
	{
		QMessageBox::warning(this, "No Classification", "The variant has to have a classification to be published!");
		return data;
	}

	// get report config
	int rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_variant WHERE id=" + QString::number(data.report_config_variant_id) + " AND variant_id="
							+ QString::number(data.variant_id), false).toInt();

	//get genes
	data.genes = db.genesOverlapping(data.variant.chr(), data.variant.start(), data.variant.end(), 5000);

	//get processed sample id
	data.processed_sample = db.processedSampleName(db.getValue("SELECT processed_sample_id FROM report_configuration WHERE id=" + QString::number(rc_id)).toString());


	return data;
}
