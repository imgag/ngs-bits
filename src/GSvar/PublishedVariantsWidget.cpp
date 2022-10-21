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
	db.replaceForeignKeyColumn(table, table.columnIndex("variant_id"), "variant", "CONCAT(chr, ':', start, '-', end, ' ', ref, '>', obs)");

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



	//filter by text
	QString text_filter = ui_->f_text->text().trimmed();
	if (text_filter!="")
	{
		table.filterRows(text_filter);
	}

	//show data
	ui_->table->setData(table, 350);

	//color results
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_red, "error");
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_green, "processed");
	ui_->table->setBackgroundColorIfEqual("ClinVar submission status", bg_yellow, "processing");

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

	QMessageBox::information(this, "ClinVar submission status updated", "The Submission status of " + QString::number(n_var_checked) + " published varaints has been checked, "
							 + QString::number(n_var_updated) + " NGSD entries were updated." );

	//activate button and reset busy curser
	ui_->updateStatus_btn->setEnabled(true);
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

		//get status
		QString status = ui_->table->item(row_idx, status_idx)->text().trimmed();
		if (status!="processed" && status!="error") //only available for already submitted variants
		{

			INFO(ArgumentException, "Reupload is only supported for variants which are submitted to ClinVar and are already processed.");
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

void PublishedVariantsWidget::openVariantTab()
{
	try
	{
		int col = ui_->table->columnIndex("variant");

		QSet<int> rows = ui_->table->selectedRows();
		foreach (int row, rows)
		{
			QString variant_text = ui_->table->item(row, col)->text();
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
		QByteArray reply = http_handler_.get("https://submit.ncbi.nlm.nih.gov/api/v1/submissions/" + submission_id.toUpper() + "/actions/", add_headers);

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
	query.prepare("SELECT sample_id, variant_id, details, user_id FROM variant_publication WHERE id=:0");
	query.bindValue(0, var_pub_id);
	query.exec();

	if (query.size() != 1) THROW(DatabaseException, "Invalid variant publication id!");
	query.next();

	QString sample_id = query.value("sample_id").toString();
	data.variant_id1 = query.value("variant_id").toInt();
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
	data.snv1 = db.variant(QString::number(data.variant_id1));

	//get variant report config id
	data.report_config_variant_id1 = -1;
	foreach (const QString& kv_pair, details)
	{
		if (kv_pair.startsWith("variant_rc_id="))
		{
			data.report_config_variant_id1 = Helper::toInt(kv_pair.split('=').at(1), "variant_rc_id");
			break;
		}
	}
	if (data.report_config_variant_id1 < 0)
	{
		THROW(DatabaseException, "No report variant config id information found in variant publication!");
	}

	//get variant report config
	query.exec("SELECT * FROM report_configuration_variant WHERE id=" + QString::number(data.report_config_variant_id1));
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
	data.report_variant_config.classification = db.getClassification(data.snv1).classification;
	if (data.report_variant_config.classification.trimmed().isEmpty() || (data.report_variant_config.classification.trimmed() == "n/a"))
	{
		QMessageBox::warning(this, "No Classification", "The variant has to have a classification to be published!");
		return data;
	}

	// get report config
	int rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_variant WHERE id=" + QString::number(data.report_config_variant_id1) + " AND variant_id="
							+ QString::number(data.variant_id1), false).toInt();

	//get genes
	data.genes = db.genesOverlapping(data.snv1.chr(), data.snv1.start(), data.snv1.end(), 5000);

	//get processed sample id
	data.processed_sample = db.processedSampleName(db.getValue("SELECT processed_sample_id FROM report_configuration WHERE id=" + QString::number(rc_id)).toString());


	return data;
}
