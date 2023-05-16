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
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include "LoginManager.h"

//TODO: change to production
const bool test_run = true;
const QString api_url = (test_run)? "https://submit.ncbi.nlm.nih.gov/apitest/v1/submissions/" : "https://submit.ncbi.nlm.nih.gov/api/v1/submissions/";

PublishedVariantsWidget::PublishedVariantsWidget(QWidget* parent)
	: QWidget(parent)
	, ui_(new Ui::PublishedVariantsWidget)
	, http_handler_(HttpRequestHandler::INI, this)
{
	ui_->setupUi(this);
	connect(ui_->search_btn, SIGNAL(clicked(bool)), this, SLOT(updateTable()));
	connect(ui_->updateStatus_btn, SIGNAL(clicked(bool)), this, SLOT(updateClinvarSubmissionStatus()));

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

	//resumit to ClinVar
	action = new QAction(QIcon(":/Icons/ClinGen.png"), "Edit/retry ClinVar submission", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(retryClinvarSubmission()));

	//resumit to ClinVar
	action = new QAction(QIcon(":/Icons/ClinGen.png"), "Delete ClinVar submission (admin-only)", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(deleteClinvarSubmission()));

	action = new QAction(QIcon(":/Icons/NGSD_variant.png"), "Open variant tab", this);
	ui_->table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openVariantTab()));
}

PublishedVariantsWidget::~PublishedVariantsWidget()
{
	delete ui_;
}

void PublishedVariantsWidget::updateTable()
{
	// define table backround colors
	QColor bg_red = Qt::red;
	bg_red.setAlphaF(0.5);
	QColor bg_green = Qt::darkGreen;
	bg_green.setAlphaF(0.5);
	QColor bg_orange = QColor(255, 135, 60);
	bg_orange.setAlphaF(0.5);
	QColor bg_yellow = Qt::yellow;
	bg_yellow.setAlphaF(0.5);
	QColor bg_gray = Qt::gray;
	bg_gray.setAlphaF(0.5);


	//init
	NGSD db;
	QStringList constraints;
	BedpeFile bedpe_struct;
	bedpe_struct.setAnnotationHeaders(QList<QByteArray>() << "QUAL" << "FILTER" << "ALT_A" << "INFO_A" << "FORMAT" << "SAMPLE");

	//filter "published by"
	if (ui_->f_published->isValidSelection())
	{
		constraints << "user_id='" + ui_->f_published->getId() + "'";
	}
	ui_->f_published->showVisuallyIfValid(true);

	//filter "gene"
	try
	{
		if (!ui_->f_gene->text().trimmed().isEmpty())
		{
			QByteArray gene = ui_->f_gene->text().trimmed().toUtf8();
			BedFile roi = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
			roi.extend(5000);
			roi.merge();
			if (roi.count()!=1 || roi.baseCount()==0) THROW(ArgumentException, "Could not convert gene to (single) region!");
			constraints << ("variant_id IN (SELECT id FROM variant where chr='" + roi[0].chr().strNormalized(true) + "' AND start>=" + QString::number(roi[0].start()) + " AND end<=" + QString::number(roi[0].end()) + ")");

			ui_->f_gene->setStyleSheet("");
		}
	}
	catch (...)
	{
		ui_->f_gene->setStyleSheet("QLineEdit {border: 2px solid red;}");
	}

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
	DBTable table = db.createTable("variant_publication", "SELECT * FROM variant_publication" + constraints_str + " ORDER BY date DESC");

	//replace foreign keys
	db.replaceForeignKeyColumn(table, table.columnIndex("sample_id"), "sample", "name");
	db.replaceForeignKeyColumn(table, table.columnIndex("user_id"), "user", "name");

	//rename columns (after keys)
	QStringList headers = table.headers();
	headers.replace(headers.indexOf("sample_id"), "sample");
	headers.replace(headers.indexOf("user_id"), "published by");
	headers.replace(headers.indexOf("variant_id"), "variant");
	table.setHeaders(headers);

	if (!ui_->cb_show_replaced->isChecked())
	{
		//remove 'replaced' column
		table.takeColumn(table.columnIndex("replaced"));
	}
	else
	{
		table.formatBooleanColumn(table.columnIndex("replaced"));
	}

	//remove 'result' column and split it into multiple columns
	int result_idx = table.columnIndex("result");
	QStringList results = table.takeColumn(result_idx);
	QStringList status, stable_ids, error_messages;
	for (int row_idx = 0; row_idx < results.size(); ++row_idx)
	{
		const QString& result = results.at(row_idx);
		if (result.isEmpty())
		{
			status << "";
			stable_ids << "";
			error_messages << "";
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
		else if (result.startsWith("deleted;"))
		{
			status << "deleted";
			stable_ids << result.split(";").at(1);
			QString message = result.split(";").at(2);
			if (result.split(";").size() > 3) message += ": " + result.split(";").at(3).trimmed();
			error_messages << message;
		}
		else
		{
			status << result;
			stable_ids << "";
			error_messages << "";
		}

	}
	table.addColumn(status, "ClinVar submission status");
	table.addColumn(stable_ids, "ClinVar accession id");
	table.addColumn(error_messages, "errors");

	//hide linked id
	table.takeColumn(table.columnIndex("linked_id"));

	//add variant description
	QStringList variant_ids = table.takeColumn(table.columnIndex("variant"));
	QStringList variant_tables = table.extractColumn(table.columnIndex("variant_table"));
	QStringList variant_details = table.extractColumn(table.columnIndex("details"));
	QStringList variant_descriptions;
	for (int i = 0; i < variant_ids.size(); ++i)
	{
		if (variant_tables.at(i) == "variant")
		{
			variant_descriptions.append(db.variant(variant_ids.at(i)).toString());
		}
		else if (variant_tables.at(i) == "n/a")
		{
			variant_descriptions.append(db.variant(variant_ids.at(i)).toString()+ " (old upload)");
		}
		else if (variant_tables.at(i) == "cnv")
		{
			variant_descriptions.append(db.cnv(variant_ids.at(i).toInt()).toString());
		}
		else if (variant_tables.at(i).startsWith("sv_"))
		{
			StructuralVariantType sv_type;
			if (variant_tables.at(i) == "sv_deletion") sv_type = StructuralVariantType::DEL;
			else if (variant_tables.at(i) == "sv_duplication") sv_type = StructuralVariantType::DUP;
			else if (variant_tables.at(i) == "sv_insertion") sv_type = StructuralVariantType::INS;
			else if (variant_tables.at(i) == "sv_inversion") sv_type = StructuralVariantType::INV;
			else if (variant_tables.at(i) == "sv_translocation") sv_type = StructuralVariantType::BND;
			else THROW(ArgumentException, "Invalid variant table name '" + variant_tables.at(i) + "'!");
			variant_descriptions.append(db.structuralVariant(variant_ids.at(i).toInt(), sv_type, bedpe_struct, true).toString());
		}
		else // none
		{
			QString variant_description;
			foreach (const QString& kv_pair, variant_details.at(i).split(';'))
			{
				if (kv_pair.startsWith("variant_desc1="))
				{
					variant_description = kv_pair.split('=').at(1) + " (manual upload)";
					break;
				}
			}
			variant_descriptions.append(variant_description);
		}
	}

	table.insertColumn(1, variant_descriptions, "variant");

	//filter by text
	QString text_filter = ui_->f_text->text().trimmed();
	if (text_filter!="")
	{
		table.filterRows(text_filter);
	}

	//show data
	ui_->table->setData(table, 350);

	//hide column
	ui_->table->hideColumn(table.columnIndex("variant_table"));

	//color results
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_red, "error");
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_green, "processed");
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_yellow, "processing");
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_gray, "deleted");

	//format replaced column if available
	if (ui_->cb_show_replaced->isChecked())
	{
		ui_->table->setBackgroundColorIfEqual("replaced", bg_gray, "yes");
	}


	//set tool tips
	ui_->table->showTextAsTooltip("errors");
	ui_->table->showTextAsTooltip("details");

	QApplication::restoreOverrideCursor();
}

void PublishedVariantsWidget::updateClinvarSubmissionStatus()
{
	//deactivate button and set busy curser
	ui_->updateStatus_btn->setEnabled(false);
	QApplication::setOverrideCursor(Qt::BusyCursor);

	// parse variant publication table and update ClinVar submission status
	NGSD db;
	SqlQuery query = db.getQuery();

	query.exec("SELECT id, details, result FROM variant_publication WHERE db='ClinVar'");

	int n_var_checked = 0;
	int n_var_updated = 0;

	while(query.next())
	{
		int vp_id = query.value("id").toInt();
		QString details = query.value("details").toString();
		QString result = query.value("result").toString();

		// skip publications without submission id
		if (!details.contains("submission_id=SUB")) continue;

		// skip publications which are already processed
		if (result.startsWith("processed")) continue;
		if (result.startsWith("error")) continue;

		//extract submission id
		QString submission_id;
		QString stable_id;

		//special handling of deletions
		if(result.startsWith("deleted;"))
		{
			bool skip = false;
			foreach (QString info, result.split(";"))
			{
				if(info.startsWith("SUB")) submission_id = info.trimmed();
				if(info.startsWith("SCV")) stable_id = info.trimmed();
				if(info.trimmed() == "processed") skip  = true;

			}
			if (submission_id.isEmpty())
			{
				THROW(ArgumentException, "'result' column doesn't contain submission id!")
			}

			//skip already deleted
			if(skip) continue;

			SubmissionStatus submission_status = getSubmissionStatus(submission_id);
			n_var_checked++;
			if (submission_status.status != result.split(";").at(2)) n_var_updated++;
			result = QStringList{"deleted", stable_id, submission_id, submission_status.status, submission_status.comment}.join(";");
			db.updateVariantPublicationResult(vp_id, result);

		}
		else
		{
			foreach (const QString& key_value_pair, details.split(';'))
			{
				if (key_value_pair.startsWith("submission_id=SUB"))
				{
					submission_id = key_value_pair.split("=").at(1).trimmed();
					break;
				}
			}
			if (submission_id.isEmpty())
			{
				THROW(ArgumentException, "'details' column doesn't contain submission id!")
			}

			SubmissionStatus submission_status = getSubmissionStatus(submission_id);
			n_var_checked++;

			//update db if neccessary
			if (!result.startsWith(submission_status.status))
			{
				//update NGSD
				result = submission_status.status;

				if (submission_status.status == "processed")
				{
					result += ";" + submission_status.stable_id;
				}
				else if (submission_status.status == "error")
				{
					result += ";" + submission_status.comment;
				}

				//update result info in the NGSD
				db.updateVariantPublicationResult(vp_id, result);
				n_var_updated++;
			}
		}





	}

	QMessageBox::information(this, "ClinVar submission status updated", "The Submission status of " + QString::number(n_var_checked) + " published varaints has been checked, "
							 + QString::number(n_var_updated) + " NGSD entries were updated." );

	//activate button and reset busy curser
	ui_->updateStatus_btn->setEnabled(true);
	QApplication::restoreOverrideCursor();

	//update search view
	updateTable();
}

void PublishedVariantsWidget::searchForVariantInLOVD()
{
	try
	{
		int var_col = ui_->table->columnIndex("variant");
		int table_col = ui_->table->columnIndex("variant_table");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_table = ui_->table->item(row, table_col)->text();
			if (variant_table != "variant") continue;
			QString variant_text = ui_->table->item(row, var_col)->text();
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
		int var_col = ui_->table->columnIndex("variant");
		int table_col = ui_->table->columnIndex("variant_table");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_table = ui_->table->item(row, table_col)->text();
			if ((variant_table != "variant") && (variant_table != "n/a")) continue;
			QString variant_text = ui_->table->item(row, var_col)->text().split('(').at(0);
			Variant variant = Variant::fromString(variant_text);
			QString url = GSvarHelper::clinVarSearchLink(variant, GSvarHelper::build());
			QDesktopServices::openUrl(QUrl(url));
		}
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "LOVD search error", e.message());
	}
}

void PublishedVariantsWidget::retryClinvarSubmission()
{
	try
	{
		QSet<int> rows = ui_->table->selectedRows();
		if (rows.size() != 1) //only available if a single line is selected
		{
			INFO(ArgumentException, "Please select exactly one varaint for re-upload!");
		}

		int row_idx = rows.values().at(0);
		int status_idx = ui_->table->columnIndex("ClinVar submission status");
		int accession_idx = ui_->table->columnIndex("ClinVar accession id");
		int variant_table_idx = ui_->table->columnIndex("variant_table");
		int details_idx = ui_->table->columnIndex("details");

		//get status
		QString status = ui_->table->item(row_idx, status_idx)->text().trimmed();
		if (status!="processed" && status!="error") //only available for already submitted variants
		{
			INFO(ArgumentException, "Reupload is only supported for variants which are submitted to ClinVar and are already processed.");
		}

		//check variant table
		QString variant_table = ui_->table->item(row_idx, variant_table_idx)->text().trimmed();
		if (variant_table=="none" || variant_table=="n/a")
		{
			INFO(ArgumentException, "Reupload is not supported for old variants and variants which were manually uploaded. \nPlease use the upload method through the GSvar sample variant view.");
		}

		//check details
		QString details = ui_->table->item(row_idx, details_idx)->text().trimmed();
		if (!details.contains("variant_rc_id"))
		{
			INFO(ArgumentException, "Reupload is not supported for old variants. \nPlease use the upload method through the GSvar sample variant view.");
		}


		// get publication id
		int var_pub_id = ui_->table->getId(row_idx).toInt();

		// get ClinVar upload data
		ClinvarUploadData data = getClinvarUploadData(var_pub_id);
		if (data.processed_sample.isEmpty())
		{
			INFO(ArgumentException,  "The ClinVar upload data is incomplete: Processed sample not set!");
		}

		//add stable id
		if (status == "processed")
		{
			data.stable_id = ui_->table->item(row_idx, accession_idx)->text();
		}

		// show dialog
		ClinvarUploadDialog dlg(this);
		dlg.setData(data);
		dlg.exec();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "ClinVar submission error");
	}
}

void PublishedVariantsWidget::deleteClinvarSubmission()
{
	//check permissions
	try
	{
		LoginManager::checkRoleIn(QStringList{"admin"});
	}
	catch (Exception& /*e*/)
	{
		QMessageBox::warning(this, "Permissions error", "Only admins can delete submissions!");
		return;
	}

	qDebug() << "Delete submission...";

	QSet<int> rows = ui_->table->selectedRows();
	if (rows.size() != 1) //only available if a single line is selected
	{
		INFO(ArgumentException, "Please select exactly one varaint for re-upload!");
	}

	int row_idx = rows.values().at(0);
	int status_idx = ui_->table->columnIndex("ClinVar submission status");
	int accession_idx = ui_->table->columnIndex("ClinVar accession id");

	// get publication id
	int var_pub_id = ui_->table->getId(row_idx).toInt();

	//get status
	QString status = ui_->table->item(row_idx, status_idx)->text().trimmed();

	//add stable id
	if (status == "processed")
	{
		// delete on ClinVar
		QString stable_id = ui_->table->item(row_idx, accession_idx)->text();

		QMessageBox::StandardButton delete_dialog;
		delete_dialog = QMessageBox::question(this, "Delete variant", "Are you sure you want to delete this variant publication?\nThe variant will also be deleted on ClinVar.",
											  QMessageBox::Yes|QMessageBox::No);
		if (delete_dialog == QMessageBox::Yes)
		{
			// create json
			QJsonObject clinvar_deletion = createJsonForClinvarDeletion(stable_id);

			// read API key
			QByteArray api_key = Settings::string("clinvar_api_key", false).toUtf8();

			QJsonObject post_request;
			QJsonArray actions;
			QJsonObject action;
			action.insert("type", "AddData");
			action.insert("targetDb", "clinvar");
			QJsonObject data;
			data.insert("content", clinvar_deletion);
			action.insert("data", data);
			actions.append(action);
			post_request.insert("actions", actions);


			// perform upload
			static HttpHandler http_handler(HttpRequestHandler::INI); //static to allow caching of credentials
			try
			{
				//switch on/off testing
				if(test_run) qDebug() << "Test run enabled!";

				QStringList messages;

				//add headers
				HttpHeaders add_headers;
				add_headers.insert("Content-Type", "application/json");
				add_headers.insert("SP-API-KEY", api_key);

				//post request
				QByteArray reply = http_handler.post(api_url, QJsonDocument(post_request).toJson(QJsonDocument::Compact), add_headers);
				qDebug() << api_url;

				// parse response
				bool success = false;
				QString submission_id;
				QJsonObject response = QJsonDocument::fromJson(reply).object();

				GUIHelper::showMessage("Response", reply);

				//successful dry-run
				if (response.isEmpty())
				{
					messages << "MESSAGE: Dry-run successful!";
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

					// update NGSD
					QString result = "deleted;" + stable_id + ";" + submission_id;
					NGSD db;
					SqlQuery query = db.getQuery();
					query.exec("UPDATE `variant_publication` SET result='" + result+ "' WHERE id=" + QString::number(var_pub_id));

					QMessageBox::information(this, "Deletion successfully submitted!", messages.join("\n"));
				}
				else
				{
					QMessageBox::critical(this, "Deletion failed!", messages.join("\n"));
				}
			}
			catch(Exception e)
			{
				QMessageBox::critical(this, "Deletion failed!", e.message());
			}
		}
	}
	else if(status == "error")
	{
		qDebug() << "delete failed submission...";
		QMessageBox::StandardButton delete_dialog;
		delete_dialog = QMessageBox::question(this, "Delete variant", "Are you sure you want to delete this variant publication?", QMessageBox::Yes|QMessageBox::No);
		if (delete_dialog == QMessageBox::Yes)
		{
			NGSD db;
			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM `variant_publication` WHERE id=" + QString::number(var_pub_id));
		}
	}
	else
	{
		QMessageBox::warning(this, "Status error", "Deletion is only supported for variants which are submitted to ClinVar and are already processed.");
		return;
	}

	//update search view
	updateTable();
}

void PublishedVariantsWidget::openVariantTab()
{
	try
	{
		int var_col = ui_->table->columnIndex("variant");
		int var_table_col = ui_->table->columnIndex("variant_table");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_text = ui_->table->item(row, var_col)->text();
			QString variant_table = ui_->table->item(row, var_table_col)->text();
			if (variant_table != "variant") continue; // skip all non-small variannts
			GlobalServiceProvider::openVariantTab(Variant::fromString(variant_text));
		}
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "Error opening variant tab", e.message());
	}
}

SubmissionStatus PublishedVariantsWidget::getSubmissionStatus(const QString& submission_id)
{
	//switch on/off testing
	if(test_run) qDebug() << "Test run enabled!";

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
		QByteArray reply = http_handler_.get(api_url + submission_id.toUpper() + "/actions/", add_headers);
		qDebug() << api_url + submission_id.toUpper() + "/actions/";
		// parse response
		QJsonObject response = QJsonDocument::fromJson(reply).object();

		//extract status
		QJsonArray actions = response.value("actions").toArray();
		submission_status.status = actions.at(0).toObject().value("status").toString();

		if (submission_status.status == "processed" || submission_status.status == "error")
		{
			//get summary file and extract stable id or error message
			QString report_summary_file = actions.at(0).toObject().value("responses").toArray().at(0).toObject().value("files").toArray().at(0).toObject().value("url").toString();
			QByteArray summary_reply = http_handler_.get(report_summary_file);
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
		QMessageBox::critical(this, "Status check failed", "Status check failed for submission " + submission_id + " (" + e.message() + ")!");

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
	query.prepare("SELECT sample_id, variant_id, variant_table, details, user_id, linked_id FROM variant_publication WHERE id=:0");
	query.bindValue(0, var_pub_id);
	query.exec();

	if (query.size() != 1) THROW(DatabaseException, "Invalid variant publication id!");
	query.next();

	QString sample_id = query.value("sample_id").toString();
	QString variant_table = query.value("variant_table").toString();
	QString variant_table2;
	data.variant_id1 = query.value("variant_id").toInt();
	QStringList details = query.value("details").toString().split(';');
	data.user_id = query.value("user_id").toInt();
	QString linked_id = query.value("linked_id").toString();


	//parse details entry
	data.report_variant_config_id1 = -1;
	data.report_variant_config_id2 = -1;
	bool switch_variants = false;
	foreach (const QString& kv_pair, details)
	{
		//determine submission type
		if (kv_pair.startsWith("submission_type="))
		{
			QString submission_type = kv_pair.split('=').at(1).trimmed();
			if (submission_type=="SingleVariant") data.submission_type = ClinvarSubmissionType::SingleVariant;
			else if(submission_type=="CompoundHeterozygous") data.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
			else THROW(DatabaseException, "Invalid submission type!");
		}

		//get variant order
		if (kv_pair.startsWith("variant_id1="))
		{
			int var_id1 = Helper::toInt(kv_pair.split('=').at(1), "variant_id1");
			switch_variants = (var_id1 != data.variant_id1);
		}
		if (kv_pair.startsWith("variant_id2="))
		{
			data.variant_id2 = Helper::toInt(kv_pair.split('=').at(1), "variant_id2");
		}

		//get variant report config id
		if (kv_pair.startsWith("variant_rc_id1="))
		{
			QString rvc_str = kv_pair.split('=').at(1);
			if (rvc_str.contains(':')) rvc_str = rvc_str.split(':').at(1);
			data.report_variant_config_id1 = Helper::toInt(rvc_str, "variant_rc_id1");
		}
		if (kv_pair.startsWith("variant_rc_id2="))
		{
			QString rvc_str = kv_pair.split('=').at(1);
			if (rvc_str.contains(':'))
			{
				variant_table2 = rvc_str.split(':').at(0);
				rvc_str = rvc_str.split(':').at(1);
			}
			data.report_variant_config_id2 = Helper::toInt(rvc_str, "variant_rc_id2");
		}

		//legacy support: get variant report config id
		if (kv_pair.startsWith("variant_rc_id="))
		{
			QString rvc_str = kv_pair.split('=').at(1);
			if (rvc_str.contains(':')) rvc_str = rvc_str.split(':').at(1);
			data.report_variant_config_id1 = Helper::toInt(rvc_str, "variant_rc_id");
		}

	}
	if (data.report_variant_config_id1 < 0) THROW(DatabaseException, "No report variant config id information found in variant publication!");
	if ((data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) && (data.report_variant_config_id2 < 0))
	{
		THROW(DatabaseException, "No report variant config id for second variant information found in variant publication!");
	}


	//get variant info
	if (variant_table == "variant")
	{
		data.variant_type1 = VariantType::SNVS_INDELS;
		data.snv1 = db.variant(QString::number(data.variant_id1));
	}
	else if(variant_table == "cnv")
	{
		data.variant_type1 = VariantType::CNVS;
		data.cnv1 = db.cnv(data.variant_id1);
	}
	else //SNV
	{
		data.variant_type1 = VariantType::SVS;
		StructuralVariantType sv_type;
		if(variant_table == "sv_deletion") sv_type = StructuralVariantType::DEL;
		else if(variant_table == "sv_duplication") sv_type = StructuralVariantType::DUP;
		else if(variant_table == "sv_insertion") sv_type = StructuralVariantType::INS;
		else if(variant_table == "sv_invertion") sv_type = StructuralVariantType::INV;
		else if(variant_table == "sv_translocation" ) sv_type = StructuralVariantType::BND;
		else THROW(ArgumentException, "Invalid SV table '" + variant_table + "'!");

		data.sv1 = db.structuralVariant(data.variant_id1, sv_type, GlobalServiceProvider::getSvList());
	}

	if (data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (linked_id == "") THROW(DatabaseException, "No linked variant provided for compound heterozygous variant publication!");
		//get publication data of second variant
		query.bindValue(0, var_pub_id);
		query.exec();
		if (query.size() != 1) THROW(DatabaseException, "Invalid variant publication id of second variant!");
		query.next();


		//get variant info for 2nd var from details field
		if (variant_table2 == "variant")
		{
			data.variant_type2 = VariantType::SNVS_INDELS;
			data.snv2 = db.variant(QString::number(data.variant_id2));
		}
		else if(variant_table2 == "cnv")
		{
			data.variant_type2 = VariantType::CNVS;
			data.cnv2 = db.cnv(data.variant_id2);
		}
		else //SNV
		{
			data.variant_type2 = VariantType::SVS;
			StructuralVariantType sv_type;
			if(variant_table2 == "sv_deletion") sv_type = StructuralVariantType::DEL;
			else if(variant_table2 == "sv_duplication") sv_type = StructuralVariantType::DUP;
			else if(variant_table2 == "sv_insertion") sv_type = StructuralVariantType::INS;
			else if(variant_table2 == "sv_invertion") sv_type = StructuralVariantType::INV;
			else if(variant_table2 == "sv_translocation" ) sv_type = StructuralVariantType::BND;
			else THROW(ArgumentException, "Invalid SV table '" + variant_table2 + "'!");

			data.sv2 = db.structuralVariant(data.variant_id2, sv_type, GlobalServiceProvider::getSvList());
		}
	}

	//switch order of variants
	if (switch_variants)
	{
		int variant_id2 = data.variant_id1;
		int report_config_variant_id2 = data.report_variant_config_id1;
		ReportVariantConfiguration report_variant_config2 = data.report_variant_config1;
		VariantType variant_type2 = data.variant_type1;
		Variant snv2 = data.snv1;
		CopyNumberVariant cnv2 = data.cnv1;
		int cn2 = data.cn1;
		int ref_cn2 = data.ref_cn1;
		BedpeLine sv2 = data.sv1;

		data.variant_id1 = data.variant_id2;
		data.report_variant_config_id1 = data.report_variant_config_id2;
		data.report_variant_config1 = data.report_variant_config2;
		data.variant_type1 = data.variant_type2;
		data.snv1 = data.snv2;
		data.cnv1 = data.cnv2;
		data.cn1 = data.cn2;
		data.ref_cn1 = data.ref_cn2;
		data.sv1 = data.sv2;

		data.variant_id2 = variant_id2;
		data.report_variant_config_id2 = report_config_variant_id2;
		data.report_variant_config2 = report_variant_config2;
		data.variant_type2 = variant_type2;
		data.snv2 = snv2;
		data.cnv2 = cnv2;
		data.cn2 = cn2;
		data.ref_cn2 = ref_cn2;
		data.sv2 = sv2;
	}

	//get sample data
	SampleData sample_data = db.getSampleData(sample_id);

	//get disease info
	data.disease_info = db.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
	data.disease_info.append(db.getSampleDiseaseInfo(sample_id, "Orpha number"));
	if (data.disease_info.length() < 1)
	{
		QMessageBox::warning(this, "No disease info", "The sample has to have at least one OMIM or Orphanet disease identifier to publish a variant in ClinVar.");
	}

	// get affected status
	data.affected_status = sample_data.disease_status;

	//get phenotype(s)
	data.phenos = sample_data.phenotypes;

	//get variant report config
	QStringList messages;
	data.report_variant_config1 = db.reportVariantConfiguration(data.report_variant_config_id1, data.variant_type1, messages);
	if (data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		data.report_variant_config2 = db.reportVariantConfiguration(data.report_variant_config_id2, data.variant_type2, messages);
	}

	if (messages.size() > 0) QMessageBox::warning(this, "Report Variant Configuration", messages.join('\n'));

	//update snv_indel classification
	if(data.variant_type1 == VariantType::SNVS_INDELS)
	{
		data.report_variant_config1.classification = db.getClassification(data.snv1).classification;
		if (data.report_variant_config1.classification.trimmed().isEmpty() || (data.report_variant_config1.classification.trimmed() == "n/a"))
		{
			QMessageBox::warning(this, "No Classification", "The variant has to have a classification to be published!");
		}
	}

	// get report config
	int rc_id = -1;
	if (data.variant_type1 == VariantType::SNVS_INDELS)
	{
		rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_variant WHERE id=" + QString::number(data.report_variant_config_id1) + " AND variant_id="
							+ QString::number(data.variant_id1), false).toInt();
	}
	else if (data.variant_type1 == VariantType::CNVS)
	{
		rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_cnv WHERE id=" + QString::number(data.report_variant_config_id1) + " AND variant_id="
							+ QString::number(data.variant_id1), false).toInt();
	}
	else if (data.variant_type1 == VariantType::SVS)
	{
		rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_sv WHERE id=" + QString::number(data.report_variant_config_id1) + " AND " + db.svTableName(data.sv1.type())
							+ "_id=" + QString::number(data.variant_id1), false).toInt();
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type!");
	}

	//get genes
	data.genes = db.genesOverlapping(data.snv1.chr(), data.snv1.start(), data.snv1.end(), 5000);

	//get processed sample id
	data.processed_sample = db.processedSampleName(db.getValue("SELECT processed_sample_id FROM report_configuration WHERE id=" + QString::number(rc_id)).toString());


	return data;
}

QJsonObject PublishedVariantsWidget::createJsonForClinvarDeletion(QString stable_id)
{
	//build up JSON
	QJsonObject json;

	//required
	QJsonObject clinvar_deletion;
	{
		//required
		QJsonObject accession_set;
		{
			accession_set.insert("accession", stable_id);
		}
		clinvar_deletion.insert("accessionSet", QJsonArray() << accession_set);
	}
	json.insert("clinvarDeletion", clinvar_deletion);

	return json;
}
