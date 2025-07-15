#include "MVHub.h"
#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "XmlHelper.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "GenLabDB.h"
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDomDocument>
#include <QPushButton>
#include <QFileDialog>
#include "ExportHistoryDialog.h"

MVHub::MVHub(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, delayed_init_(this, true)
{
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableContextMenu(QPoint)));
	connect(ui_.f_text, SIGNAL(textChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_network, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_messages, SIGNAL(stateChanged(int)), this, SLOT(updateTableFilters()));
	connect(ui_.f_ready_export, SIGNAL(stateChanged(int)), this, SLOT(updateTableFilters()));
	connect(ui_.load_consent_data, SIGNAL(clicked()), this, SLOT(loadConsentData()));
	connect(ui_.load_genlab_data, SIGNAL(clicked()), this, SLOT(loadGenLabData()));
	connect(ui_.export_consent_data, SIGNAL(clicked()), this, SLOT(exportConsentData()));
	connect(ui_.check_xml, SIGNAL(clicked()), this, SLOT(checkXML()));
}

void MVHub::delayedInitialization()
{
	loadDataFromCM();
	loadDataFromSE();

	determineProcessedSamples();
	updateExportStatus();

	showMessages();

	//resize columns
	GUIHelper::resizeTableCellWidths(ui_.table, 400, -1);
	GUIHelper::resizeTableCellHeightsToMinimum(ui_.table, 20);
}

void MVHub::tableContextMenu(QPoint pos)
{
	QList<int> rows = GUIHelper::selectedTableRows(ui_.table);

	//create menu
	QMenu menu;
	QAction* a_copy = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all");
	QAction* a_copy_sel = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection");
	menu.addSeparator();
	QAction* a_show_ngsd = menu.addAction("Show all NGSD samples");
	a_show_ngsd->setEnabled(rows.count()==1);
	menu.addSeparator();
	QAction* a_export = menu.addAction("Export to GRZ/KDK");
	a_export->setEnabled(ui_.f_ready_export->isChecked() && rows.count()==1); //TODO batch export
	QAction* a_export_history = menu.addAction("Show export history");
	a_export_history->setEnabled(rows.count()==1);

	//execute
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	//actions
	if (action==a_copy)
	{
		GUIHelper::copyToClipboard(ui_.table);
	}
	if (action==a_copy_sel)
	{
		GUIHelper::copyToClipboard(ui_.table, true);
	}
	if (action==a_show_ngsd)
	{
		NGSD db;
		GenLabDB genlab;

		int c_sap = GUIHelper::columnIndex(ui_.table, "SAP ID");
		QString sap_id = getString(rows.first(), c_sap);
		QString title = "Samples of case with SAP ID "+sap_id;

		//get processed samples
		ProcessedSampleSearchParameters params;
		params.run_finished = true;
		params.p_type = "diagnostic";
		params.include_bad_quality_samples = true;
		params.include_tumor_samples = true;
		QStringList ps_list = genlab.samplesWithSapID(sap_id, params);
		if (ps_list.isEmpty())
		{
			QMessageBox::information(this, title, "No samples found in NGSD!");
			return;
		}

		//show table
		QTableWidget* table = new QTableWidget();
		table->setMinimumSize(1000, 600);
		QStringList cols = QStringList() << "PS" << "quality" << "tumor/ffpe" << "system" << "project" << "run";
		table->setColumnCount(cols.count());
		table->setHorizontalHeaderLabels(cols);
		table->setRowCount(ps_list.count());
		int r = 0;
		foreach(QString ps, ps_list)
		{
			//get NGSD infos
			QString ps_id = db.processedSampleId(ps);
			ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
			QString s_id = db.sampleId(ps);
			SampleData s_data = db.getSampleData(s_id);

			table->setItem(r, 0, GUIHelper::createTableItem(ps));
			table->setItem(r, 1, GUIHelper::createTableItem(ps_data.quality));
			table->setItem(r, 2, GUIHelper::createTableItem(QString(s_data.is_tumor ? "yes" : "no") + "/" + (s_data.is_ffpe ? "yes" : "no")));
			table->setItem(r, 3, GUIHelper::createTableItem(ps_data.processing_system_type + " ("+ps_data.processing_system+")"));
			table->setItem(r, 4, GUIHelper::createTableItem(ps_data.project_name + " (" + ps_data.project_type +")"));
			table->setItem(r, 5, GUIHelper::createTableItem(ps_data.run_name + " (" + db.getValue("SELECT start_date FROM sequencing_run WHERE name='"+ps_data.run_name+"'").toString()+")"));

			++r;
		}
		GUIHelper::resizeTableCellWidths(table, 500);
		QSharedPointer<QDialog> dlg = GUIHelper::createDialog(table, title);
		dlg->exec();
	}
	if (action==a_export)
	{
		try
		{
			//init
			int r = rows.first();
			QString title = "GRZ/KDK export";
			NGSD mvh_db(true, "mvh");
			int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
			int c_sap = GUIHelper::columnIndex(ui_.table, "SAP ID");

			//get ID
			QString cm_id = getString(r, c_cm);
			QByteArray id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+cm_id+"'", false).toByteArray().trimmed();

			//check that GRZ export is not pending/done
			int pending_grz = mvh_db.getValue("SELECT count(*) FROM submission_grz WHERE case_id='"+id+"' and status='pending'").toInt();
			int done_grz = mvh_db.getValue("SELECT count(*) FROM submission_grz WHERE case_id='"+id+"' and status='done'").toInt();
			if (pending_grz>0)
			{
				QMessageBox::warning(this, title, "GRZ export is pending. Be patient...");
			}
			else if (done_grz>0)
			{
				QMessageBox::warning(this, title, "GRZ export is already done!"); //TODO implement followup/addition/correction
			}
			else //add GRZ export
			{
				QByteArray tag = getString(r, c_sap).toLatin1() + "_" + QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1();
				QString tan = getPseudonym(tag, "GRZ", false, false);
				mvh_db.getQuery().exec("INSERT INTO `submission_grz`(`case_id`, `date`, `type`, `tang`, `status`) VALUES ("+id+",CURDATE(),'initial','"+tan+"','pending')");
				updateExportStatus(mvh_db, r);
			}

			//check that KDK export is not pending/done
			int pending_kdk = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='"+id+"' and status='pending'").toInt();
			int done_kdk = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='"+id+"' and status='done'").toInt();
			if (pending_kdk>0)
			{
				QMessageBox::warning(this, title, "KDK export is pending. Be patient...");
			}
			else if (done_kdk>0)
			{
				QMessageBox::warning(this, title, "KDK export is already done!"); //TODO implement followup/addition/correction
			}
			else //add KDK export
			{
				QByteArray tag = getString(r, c_sap).toLatin1() + "_" + QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1();
				QString tan = getPseudonym(tag, "KDK_SE", false, false);
				mvh_db.getQuery().exec("INSERT INTO `submission_kdk_se`(`case_id`, `date`, `type`, `tank`, `status`) VALUES ("+id+",CURDATE(),'initial','"+tan+"','pending')");
				updateExportStatus(mvh_db, r);
			}
		}
		catch (Exception& e)
		{
			ui_.output->appendPlainText("Export error:\n"+e.message());
		}
	}
	if (action==a_export_history)
	{
		int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
		int c_network = GUIHelper::columnIndex(ui_.table, "Netzwerk");
		int r = rows.first();

		ExportHistoryDialog dlg(this, getString(r, c_cm), getString(r, c_network));
		dlg.exec();
	}
}

void MVHub::updateTableFilters()
{
	const int rows = ui_.table->rowCount();
	const int cols = ui_.table->columnCount();
	QBitArray visible(rows, true);


	//apply network filter
	QString f_network = ui_.f_network->currentText();
	if (!f_network.isEmpty())
	{
		int c_network = GUIHelper::columnIndex(ui_.table, "Netzwerk");
		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			visible[r] = getString(r, c_network)==f_network;
		}
	}

	//apply text filter
	QString f_text = ui_.f_text->text().trimmed();
	if (!f_text.isEmpty())
	{
		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			bool string_match = false;
			for (int c=0; c<cols; ++c)
			{
				if (getString(r,c).contains(f_text, Qt::CaseInsensitive))
				{
					string_match = true;
					break;
				}
			}
			if (!string_match) visible[r] = false;
		}
	}

	//apply messages filter
	if (ui_.f_messages->isChecked())
	{
		int c_messages = GUIHelper::columnIndex(ui_.table, "messages");

		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			if (getString(r, c_messages).trimmed().isEmpty())
			{
				visible[r] = false;
			}
		}
	}

	//apply GRZ/KDK export filter
	if (ui_.f_ready_export->isChecked())
	{
		int c_network = GUIHelper::columnIndex(ui_.table, "Netzwerk");
		int c_seq_type = GUIHelper::columnIndex(ui_.table, "Sequenzierungsart");
		int c_network_id = GUIHelper::columnIndex(ui_.table, "Netzwerk ID");
		int c_consent = GUIHelper::columnIndex(ui_.table, "consent");
		int c_genlab = GUIHelper::columnIndex(ui_.table, "GenLab");
		int c_report_date = GUIHelper::columnIndex(ui_.table, "Befunddatum");
		int c_te_retracted = GUIHelper::columnIndex(ui_.table, "Kündigung TE");

		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			if (getString(r, c_network)!="Netzwerk Seltene Erkrankungen") //TODO implement for other networks
			{
				visible[r] = false;
				continue;
			}

			if (getString(r, c_seq_type)=="Keine") //TODO implement as well
			{
				visible[r] = false;
				continue;
			}

			//SE - report done, all data available and TE not retreacted
			if (getString(r, c_report_date)=="" || getString(r, c_network_id)=="" || getString(r, c_consent)=="" || getString(r, c_genlab)=="" || getString(r, c_te_retracted)!="")
			{
				visible[r] = false;
				continue;
			}
		}
	}

	//set row visiblity
	for (int r=0; r<rows; ++r)
	{
		ui_.table->setRowHidden(r, !visible[r]);
	}

	ui_.filter_status->setText(QString::number(visible.count(true))+"/"+QString::number(rows)+" pass");
}

void MVHub::loadConsentData()
{
	addOutputHeader("loading consent data");

	int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
	int c_sap = GUIHelper::columnIndex(ui_.table, "SAP ID");
	int c_consent = GUIHelper::columnIndex(ui_.table, "consent");
	if (ui_.table->columnWidth(c_consent)<300) ui_.table->setColumnWidth(c_consent, 300);

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString cm_id = getString(r, c_cm);
		QString sap_id = getString(r, c_sap);
		QString consent = getConsent(sap_id);
		QString consent_json = getConsent(sap_id, false);

		//store consent data in MVH database
		NGSD mvh_db(true, "mvh");
		SqlQuery query = mvh_db.getQuery();
		query.prepare("UPDATE case_data SET rc_data=:0, rc_data_json=:1 WHERE cm_id=:2");
		query.bindValue(0, consent);
		query.bindValue(1, consent_json);
		query.bindValue(2, cm_id);
		query.exec();


		QTableWidgetItem* item = GUIHelper::createTableItem(consent.isEmpty() ? "" : "yes (see tooltip)");
		item->setToolTip(consent);
		ui_.table->setItem(r, c_consent, item);

		qApp->processEvents();
	}

	ui_.output->appendPlainText("");
	ui_.output->appendPlainText("done");
}

void MVHub::loadGenLabData()
{
	addOutputHeader("loading GenLab data");

	GenLabDB genlab;
	int c_ps = GUIHelper::columnIndex(ui_.table, "PS");
	int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
	int c_genlab = GUIHelper::columnIndex(ui_.table, "GenLab");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString ps = getString(r, c_ps);
		if (ps.isEmpty()) continue;

		ui_.output->appendPlainText("Getting GenLab data for "+ps+"...");

		QStringList gl_data;
		gl_data << "<item>";
		gl_data << "  <accounting_mode>" + genlab.accountingData(ps).accounting_mode + "</accounting_mode>";
		gl_data << "</item>";

		QString gl_string = gl_data.join("\n");

		//store GenLab data in MVH database
		QString cm_id = getString(r, c_cm);
		NGSD mvh_db(true, "mvh");
		SqlQuery query = mvh_db.getQuery();
		query.prepare("UPDATE case_data SET gl_data=:0 WHERE cm_id=:1");
		query.bindValue(0, gl_string);
		query.bindValue(1, cm_id);
		query.exec();

		QTableWidgetItem* item = GUIHelper::createTableItem("yes (see tooltip)");
		item->setToolTip(gl_string);
		ui_.table->setItem(r, c_genlab, item);

		qApp->processEvents();
	}

	ui_.table->resizeColumnToContents(c_genlab);

	ui_.output->appendPlainText("");
	ui_.output->appendPlainText("done");
}

void MVHub::exportConsentData()
{
	QString title = "Import SAP IDs.";
	//get file names
	QString filename = QFileDialog::getOpenFileName(this, "Text file with one SAP patient identifier per line, or in first column of TSV file");
	if(filename.isEmpty()) return;

	//create ouput
	QStringList output;
	foreach(QString line, Helper::loadTextFile(filename, true, '#', true))
	{
		QStringList parts = line.split('\t');
		QString sap_id = parts[0];

		output << "";
		output << ("###" + sap_id + "###");
		output << getConsent(sap_id, false);
		output << "";
	}

	//store output
	QString filename2 = QFileDialog::getSaveFileName(this, "File to store consent data");
	if(filename2.isEmpty()) return;
	auto file_handle = Helper::openFileForWriting(filename2);
	Helper::storeTextFile(file_handle, output);
}

void MVHub::checkXML()
{
	addOutputHeader("checking XML data");

	//init
	NGSD mvh_db(true, "mvh");
	QString tmp_file = Helper::tempFileName(".xml");

	//iterate over all rows
	SqlQuery query = mvh_db.getQuery();
	query.exec("SELECT id, cm_data, se_data, rc_data, gl_data FROM case_data");
	while(query.next())
	{
		QString id = query.value("id").toString();

		//iterate over all fields
		foreach(QString field, QStringList() << "cm_data" << "se_data" << "rc_data" << "gl_data")
		{
			QString data = query.value(field).toString().trimmed();
			if (data.isEmpty()) continue;

			try
			{
				Helper::storeTextFile(tmp_file, QStringList() << "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>" << data.split("\n"));
				QString error = XmlHelper::isValidXml(tmp_file);
				if (!error.isEmpty())
				{
					ui_.output->appendPlainText(id +"\t" + field + "\t" + error);
				}
			}
			catch (Exception& e)
			{
				ui_.output->appendPlainText(id +"\t" + field + "\t" + e.message());
			}
		}
	}
	addOutputHeader("checking XML data: done", false);
}

void MVHub::determineProcessedSamples()
{
	//init
	NGSD db;
	GenLabDB genlab;
	NGSD mvh_db(true, "mvh");
	int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
	int c_sap = GUIHelper::columnIndex(ui_.table, "SAP ID");
	int c_network = GUIHelper::columnIndex(ui_.table, "Netzwerk");
	int c_seq_type = GUIHelper::columnIndex(ui_.table, "Sequenzierungsart");
	int c_report = GUIHelper::columnIndex(ui_.table, "Befunddatum");
	int c_ps = GUIHelper::columnIndex(ui_.table, "PS");
	int c_ps_t = GUIHelper::columnIndex(ui_.table, "PS tumor");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		//skip if already in MVH database
		QString cm_id = getString(r, c_cm);
		QString ps_in_mvh_db = mvh_db.getValue("SELECT ps FROM case_data WHERE cm_id='"+cm_id+"'").toString().trimmed();
		if(ps_in_mvh_db!="")
		{
			ui_.table->setItem(r, c_ps, GUIHelper::createTableItem(ps_in_mvh_db));
			ui_.table->setItem(r, c_ps_t, GUIHelper::createTableItem(mvh_db.getValue("SELECT ps_t FROM case_data WHERE cm_id='"+cm_id+"'").toString().trimmed()));
			continue;
		}

		//determine search parameters
		ProcessedSampleSearchParameters params;
		params.run_finished = true;
		params.p_type = "diagnostic";
		params.r_after = QDate(2024, 7, 1);

		//skip if information is not complete
		QString report_date = getString(r, c_report);
		if (report_date.isEmpty()) continue;
		QString seq_type = getString(r, c_seq_type);
		QString network = getString(r, c_network);
		if (network.isEmpty() || seq_type.isEmpty() || seq_type=="Keine") continue;

		if (network=="Netzwerk Seltene Erkrankungen" && seq_type=="WGS")
		{
			params.include_bad_quality_samples = false;
			params.include_tumor_samples = false;
			//system type can be WGS or lrGS > filtering below
		}
		else
		{
			cmid2messages_[cm_id] << "Could not determine processed sample(s): unhandled combination of network '"+network+"' and sequencing type '"+seq_type+"'";
		}

		QString sap_id = getString(r, c_sap);
		QStringList ps_list;
		QStringList tmp = genlab.samplesWithSapID(sap_id, params);
		foreach(QString ps, tmp)
		{
			QString ps_id = db.processedSampleId(ps);
			ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
			QString sys_type = ps_data.processing_system_type;
			if (seq_type=="WGS" && sys_type!="WGS" && sys_type!="lrGS") continue; //system type can be WGS or lrGS > filtering here instead via parameters

			QString entry = ps + " ("+sys_type+")";
			ps_list << entry;
		}
		if (ps_list.count()==0)
		{
			cmid2messages_[cm_id] << "Could not determine/store processed sample(s): no matching samples in NGSD";
		}
		else if (ps_list.count()>1)
		{
			cmid2messages_[cm_id] << "Could not determine/store processed sample(s): several matching samples in NGSD";
		}
		else
		{
			QString ps = ps_list.first().split(" ").first();
			ui_.table->setItem(r, c_ps, GUIHelper::createTableItem(ps));

			//store PS in MVH database
			SqlQuery query = mvh_db.getQuery();
			query.exec("UPDATE case_data SET ps='"+ps+"' WHERE cm_id='"+cm_id+"'");
		}
	}

	//TODO replace by view v_ngs_modellvorhaben
	//check if PS are in the correct study in NGSD
	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString cm_id = getString(r, c_cm);
		QString ps = getString(r, c_ps).split(" ").first();
		if (ps!="")
		{
			QString ps_id = db.processedSampleId(ps);
			if (!db.studies(ps_id).contains("Modellvorhaben_2024"))
			{
				cmid2messages_[cm_id] << (ps +" not in study");
			}
		}

		QString ps_t = getString(r, c_ps_t);
		if (ps_t!="")
		{
			QString ps_id = db.processedSampleId(ps_t).split(" ").first();
			if (!db.studies(ps_id).contains("Modellvorhaben_2024"))
			{
				cmid2messages_[cm_id] << (ps_t +" not in study");
			}
		}
	}
}

void MVHub::updateExportStatus()
{
	//init
	NGSD mvh_db(true, "mvh");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		updateExportStatus(mvh_db, r);
	}
}


void MVHub::updateExportStatus(NGSD& mvh_db, int r)
{
	//init
	int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
	int c_export_status = GUIHelper::columnIndex(ui_.table, "export status");

	//get ID
	QString cm_id = getString(r, c_cm);
	QString id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+cm_id+"'", false).toString().trimmed();

	//determine overall export count
	int c_exp_grz = mvh_db.getValue("SELECT count(*) FROM submission_grz WHERE case_id='" + id + "'").toInt();
	int c_exp_kdk = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='" + id + "'").toInt();
	int c_exp_all = c_exp_grz + c_exp_kdk;
	if (c_exp_all==0) return;


	QString text = "uploads: " + QString::number(c_exp_all);

	//add status of latest GRZ upload
	SqlQuery query = mvh_db.getQuery();
	query.exec("SELECT * FROM submission_grz WHERE case_id='" + id + "' ORDER BY id DESC LIMIT 1");
	if(query.next())
	{
		text += " // GRZ " + query.value("status").toString();
	}

	//add status of latest KDK upload
	query.exec("SELECT * FROM submission_kdk_se WHERE case_id='" + id + "' ORDER BY id DESC LIMIT 1");
	if(query.next())
	{
		text += " // KDK " + query.value("status").toString();
	}

	//add table item
	QTableWidgetItem* item = GUIHelper::createTableItem(text);
	if (!item->text().contains("pending")) item->setForeground(Qt::darkGreen);
	if (item->text().contains("failed")) item->setForeground(Qt::red);
	ui_.table->setItem(r, c_export_status, item);
}

void MVHub::showMessages()
{
	int c_cm = GUIHelper::columnIndex(ui_.table, "CM ID");
	int c_messages = GUIHelper::columnIndex(ui_.table, "messages");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString cm_id = getString(r,c_cm);
		if (!cmid2messages_.contains(cm_id)) continue;

		QString text = cmid2messages_[cm_id].join("\n");
		QTableWidgetItem* item = GUIHelper::createTableItem(text);
		item->setToolTip(text);
		ui_.table->setItem(r, c_messages, item);
	}
}

QByteArray MVHub::jsonDataPseudo(QByteArray str)
{
	return	QByteArray( "{ \"resourceType\": \"Bundle\", ")+
						"  \"entry\": ["+
						"	{"+
						"		\"resource\": {"+
						"			\"resourceType\": \"Encounter\","+
						"			\"identifier\": ["+
						"				{"+
						"					\"use\": \"official\","+
						"					\"system\": \"FallID\","+
						"					\"value\": \"" + str + "\""+
						"				}"+
						"			]"+
						"		}"+
						"	}"+
						"]"+
			"}";
}

QByteArray MVHub::parseJsonDataPseudo(QByteArray reply, QByteArray context)
{
	QJsonDocument doc = QJsonDocument::fromJson(reply);
	QJsonArray entries = doc.object()["entry"].toArray();
	for (int i=0; i<entries.count(); ++i)
	{
		QJsonObject object = entries[i].toObject();
		if (!object.contains("resource")) continue;

		object = object["resource"].toObject();
		if (!object.contains("resourceType")) continue;
		if (object["resourceType"].toString()!="Encounter") continue;

		//get base64-encoded string from results
		QByteArray str_base64 = object["identifier"].toArray()[0].toObject()["value"].toString().toLatin1().trimmed();

		//use webserice to decrypt (not easy in C++)
		QString url = Settings::string("pseudo_decrypt_webservice");

		QFile file(Settings::string("pseudo_key_"+context));
		file.open(QIODevice::ReadOnly);
		QByteArray data = file.readAll();
		file.close();

		//ui_.output->appendPlainText("base64-encoded key: " +data.toBase64());
		HttpHandler handler(true);
		QByteArray reply = handler.post(url, "input="+str_base64+"&key="+data.toBase64());

		return reply;
	}

	THROW(ArgumentException, "Could not parse JSON for pseudonymization: " + reply);
}

QString MVHub::getString(int r, int c, bool trim)
{
	QString output;

	QTableWidgetItem* item = ui_.table->item(r, c);
	if (item==nullptr) return output;
	output = item->text();

	if (trim) output = output.trimmed();

	return output;
}

void MVHub::addOutputHeader(QString section, bool clear)
{
	if (clear) ui_.output->clear();

	if (!section.isEmpty())
	{
		ui_.output->appendPlainText("### " + section + " ###");
		ui_.output->appendPlainText("");
	}
}

QString MVHub::getConsent(QString sap_id, bool return_parsed_data, bool debug)
{
	static QByteArray token = "";
	try
	{
		//meDIC API expects SAP ID padded to 10 characters...
		sap_id = sap_id.trimmed();
		while (sap_id.size()<10) sap_id.prepend('0');

		//test or production
		bool test_server = false;
		if (test_server)
		{
			ui_.output->appendPlainText("ATTENTION: using test servers for getConsent");
			ui_.output->appendPlainText("");
		}

		//get token if missing
		if (token.isEmpty())
		{
			ui_.output->appendPlainText("");
			ui_.output->appendPlainText("Getting consent token...");
			HttpHeaders headers;
			headers.insert("Content-Type", "application/x-www-form-urlencoded");
			headers.insert("Prefer", "handling=strict");
			QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("consent_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("consent_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
			QString url = test_server ? "https://tc-t.med.uni-tuebingen.de/auth/realms/consent-test/protocol/openid-connect/token" : "https://tc-p.med.uni-tuebingen.de/auth/realms/consent/protocol/openid-connect/token";
			if (debug) ui_.output->appendPlainText("URL: "+url);
			if (debug) ui_.output->appendPlainText("data: " + data);
			HttpHandler handler(true);
			QByteArray reply = handler.post(url, data, headers);
			QByteArrayList parts = reply.split('"');
			if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

			token = parts[3];
			if (debug) ui_.output->appendPlainText("Token: "+ token);
		}

		//get consent
		ui_.output->appendPlainText("Getting consent for "+sap_id+"...");
		HttpHeaders headers2;
		headers2.insert("Authorization", "Bearer "+token);
		QString url = test_server ? "https://tc-t.med.uni-tuebingen.de:8443/fhir/Consent" : "https://tc-p.med.uni-tuebingen.de:8443/fhir/Consent";
		url += "?patient.identifier="+sap_id;
		HttpHandler handler(true);
		QByteArray reply = handler.get(url, headers2);
		if (debug)
		{
			ui_.output->appendPlainText("URL: " + url);
			ui_.output->appendPlainText("SAP ID: " + sap_id);
			ui_.output->appendPlainText("Consent: " + reply);
		}

		return return_parsed_data ? parseConsentJson(reply) : reply;
	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());

		//clear token in case of error
		token = "";
	}

	return "";
}

QByteArray MVHub::parseConsentJson(QByteArray json_text)
{
	QByteArrayList output;

	QJsonDocument doc = QJsonDocument::fromJson(json_text);
	QJsonArray entries = doc.object()["entry"].toArray();
	for (int i=0; i<entries.count(); ++i)
	{
		QJsonObject object = entries[i].toObject();
		if (!object.contains("resource")) continue;

		object = object["resource"].toObject();
		if (!object.contains("resourceType")) continue;
		if (object["resourceType"].toString()!="Consent") continue;

		//remove consents before Modellvorhaben
		QDate date = QDate::fromString(object["dateTime"].toString().left(10), Qt::ISODate);
		if (date<QDate(2024, 7, 1)) continue;

		QByteArrayList allowed;
		QJsonArray provisions = object["provision"].toObject()["provision"].toArray();
		for (int i=0; i<provisions.count(); ++i)
		{
			QJsonObject object = provisions[i].toObject();

			if (!object.contains("type")) continue;
			if (object["type"].toString()!="permit") continue;

            for (const QJsonValue& v : object["code"].toArray())
			{
                for (const QJsonValue& v2 : v.toObject()["coding"].toArray())
				{
					allowed << "    <permit>";
					allowed << "      <code>"+v2.toObject()["code"].toString().toLatin1()+"</code>";
					allowed << "      <display>"+v2.toObject()["display"].toString().toLatin1()+"</display>";
					allowed << "    </permit>";
				}
			}
		}

		QByteArray status = object["status"].toString().toLatin1();
		QByteArray start = object["provision"].toObject()["period"].toObject()["start"].toString().left(10).toUtf8();
		QByteArray end = object["provision"].toObject()["period"].toObject()["end"].toString().left(10).toUtf8();
		output << "  <consent>";
		output << "    <date>"+date.toString(Qt::ISODate).toLatin1()+"</date>";
		output << "    <status>"+status+"</status>";
		output << "    <start>"+start+"</start>";
		output << "    <end>"+end+"</end>";
		output << allowed;
		output << "  </consent>";
	}

	if (!output.isEmpty())
	{
		output.prepend("<consents>");
		output.append("</consents>");
	}

	return output.join("\n");
}

QByteArray MVHub::getPseudonym(QByteArray str, QByteArray context, bool test_server, bool debug)
{
	//check context
	if (context!="GRZ" && context!="KDK_SE")
	{
		THROW(ProgrammingException, " MVHub::getPseudonym: unknown context '" + context + "'!");
	}

	//test or production
	if (test_server)
	{
		ui_.output->appendPlainText("ATTENTION: using test server!");
		ui_.output->appendPlainText("");
	}

	//get token
	QByteArray token = "";
	{
		if (debug) ui_.output->appendPlainText("Getting token...");

		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		headers.insert("Prefer", "handling=strict");

		QString url = "https://" + QString(test_server ? "tc-t" : "tc-p") + ".med.uni-tuebingen.de/auth/realms/trustcenter/protocol/openid-connect/token";
		if (debug) ui_.output->appendPlainText("URL: "+url);
		QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("pseudo_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("pseudo_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
		if (debug) ui_.output->appendPlainText("data: " + data);

		HttpHandler handler(true);
		QByteArray reply = handler.post(url, data, headers);
		QByteArrayList parts = reply.split('"');
		if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

		token = parts[3];
		if (debug) ui_.output->appendPlainText("Token: " + token);
	}

	//get first pseudonyms
	if (debug) ui_.output->appendPlainText("");
	if (debug) ui_.output->appendPlainText("String to encode: " + str);
	QByteArray pseudo1 = "";
	{
		QString url = "https://tc-" + QString(test_server ? "t" : "p") + ".med.uni-tuebingen.de/v1/process?targetSystem=MVH_"+(test_server ? "T" : "P")+"_F";
		if (debug) ui_.output->appendPlainText("URL: "+url);

		HttpHeaders headers;
		headers.insert("Content-Type", "application/json");
		headers.insert("Authorization", "Bearer "+token);

		HttpHandler handler(true);
		QByteArray data =  jsonDataPseudo(str);
		if (debug) ui_.output->appendPlainText("data: " + data);
		QByteArray reply = handler.post(url, data, headers);
		if (debug) ui_.output->appendPlainText("reply: " + reply);
		pseudo1 = parseJsonDataPseudo(reply, "first_level");
	}
	if (debug) ui_.output->appendPlainText("Pseudonym 1: " + pseudo1);

	//get second pseudonyms
	QByteArray pseudo2 = "";
	{
		QString url = "https://tc-" + QString(test_server ? "t" : "p") + ".med.uni-tuebingen.de/v1/process?targetSystem=MVH_"+(test_server ? "T" : "P")+"_"+(context=="GRZ" ? "G" : "SE");
		if (debug) ui_.output->appendPlainText("URL: "+url);

		HttpHeaders headers;
		headers.insert("Content-Type", "application/json");
		headers.insert("Authorization", "Bearer "+token);

		HttpHandler handler(true);
		QByteArray data =  jsonDataPseudo(pseudo1);
		if (debug) ui_.output->appendPlainText("data: " + data);
		QByteArray reply = handler.post(url, data, headers);
		pseudo2 = parseJsonDataPseudo(reply, "second_level");
	}
	if (debug) ui_.output->appendPlainText("Pseudonym 2: " + pseudo2);

	return pseudo2;
}

void MVHub::loadDataFromCM()
{
	try
	{
		addOutputHeader("loading case-management data from RedCap", false);

		QHash<QString, QStringList> cmid2data;
		QHash<QString, QString> cmid2sapid;

		//process data from RedCap API
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		QByteArray data = "token="+Settings::string("redcap_casemanagement_token").toLatin1()+"&content=record&rawOrLabel=label";
		HttpHandler handler(true);
		QByteArrayList reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers).split('\n');
		foreach(QString line, reply)
		{
			if (!line.startsWith("<item>")) continue;

			//open file
			QDomDocument doc;
			QString error_msg;
			int error_line, error_column;
			if(!doc.setContent(line, &error_msg, &error_line, &error_column))
			{
				THROW(FileParseException, "XML invalid: " + error_msg + " line: " + QString::number(error_line) + " column: " +  QString::number(error_column));
			}

			QDomElement root = doc.documentElement();
			QString cm_id = root.namedItem("record_id").toElement().text().trimmed();
			cmid2data[cm_id] << line;
			QString sap_id = root.namedItem("pat_id").toElement().text().trimmed();
			if (!sap_id.isEmpty()) cmid2sapid[cm_id] = sap_id; //redcap_repeat_instance SAP ID is empty

			//skip items flagged as 'redcap_repeat_instance'
			if(root.namedItem("redcap_repeat_instance").toElement().text()=="1") continue;

			//add table line
			int r = ui_.table->rowCount();
			ui_.table->setRowCount(r+1);

			//fill table line
			QDomNode n = root.firstChild();
			while(!n.isNull())
			{
				QDomElement e = n.toElement(); // try to convert the node to an element.
				if(e.isNull()) continue;

				QString tag = e.tagName();
				if (tag=="record_id") ui_.table->setItem(r, 0, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="pat_id") ui_.table->setItem(r, 1, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="network_title") ui_.table->setItem(r, 2, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="seq_mode") ui_.table->setItem(r, 4, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="sample_arrival_date") ui_.table->setItem(r, 5, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="seq_state") ui_.table->setItem(r, 6, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="gen_finding_date") ui_.table->setItem(r, 7, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="datum_kuendigung_te") ui_.table->setItem(r, 8, GUIHelper::createTableItem(e.text().trimmed()));

				n = n.nextSibling();
			}
		}

		//insert/update data in MVH database
		NGSD mvh_db(true, "mvh");
		SqlQuery query = mvh_db.getQuery();
		query.prepare("INSERT INTO case_data (cm_id, cm_data, sap_id) VALUES (:0,:1,:2) ON DUPLICATE KEY UPDATE cm_data=VALUES(cm_data), sap_id=VALUES(sap_id)");
		foreach(QString cm_id, cmid2sapid.keys())
		{
			QString sap_id = cmid2sapid[cm_id];

			//check that SAP ID is not used by other sample
			QString mvh_case_id = mvh_db.getValue("SELECT cm_id FROM case_data WHERE sap_id='"+sap_id+"'").toString();
			if (!mvh_case_id.isEmpty() && mvh_case_id!=cm_id)
			{
				ui_.output->appendPlainText("Skipping sample with CM ID '" + cm_id + "': SAP ID '"+sap_id+"' already used by sample with CM ID '" + mvh_case_id + "'!");
				continue;
			}

			//insert data
			QStringList cm_data;
			cm_data << "<items>";
			cm_data << cmid2data[cm_id];
			cm_data << "</items>";
			query.bindValue(0, cm_id);
			query.bindValue(1, cm_data.join("\n"));
			query.bindValue(2, sap_id);
			query.exec();
		}
	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());
	}
}

void MVHub::loadDataFromSE()
{
	try
	{
		addOutputHeader("loading SE data from RedCap", false);

		QTextStream stream(stdout);
		QHash<QString, QString> sap2psn;
		QHash<QString, QString> psn2sap;
		QHash<QString, QStringList> psn2items;

		//get data from RedCap API
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		QByteArray data = "token="+Settings::string("redcap_se_token").toLatin1()+"&content=record&rawOrLabel=label";
		HttpHandler handler(true);
		QByteArrayList reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers).split('\n');

		//parse items
		QDomDocument doc;
		QString error_msg;
		int error_line, error_column;
		if(!doc.setContent(reply.join("\n"), &error_msg, &error_line, &error_column))
		{
			THROW(FileParseException, "XML invalid: " + error_msg + " line: " + QString::number(error_line) + " column: " +  QString::number(error_column));
		}
		QDomElement root = doc.documentElement();
		QDomNode n = root.firstChild();
		while(!n.isNull())
		{
			QDomElement e = n.toElement(); // try to convert the node to an element.
			if(e.isNull()) continue;

			QString psn_id = e.namedItem("psn").toElement().text().trimmed();
			QString sap_id = e.namedItem("pat_id").toElement().text().trimmed();

			if (!sap_id.isEmpty()) psn2sap[psn_id] = sap_id;

			QString item;
			QTextStream stream(&item);
			e.save(stream, 0);
			stream.flush();
			psn2items[psn_id] << item;

			n = n.nextSibling();
		}

		//store data in MVH database
		for(auto it=psn2sap.begin(); it!=psn2sap.end(); ++it)
		{
			QString psn_id = it.key();
			QString sap_id = it.value();
			if (sap_id.isEmpty())
			{
				ui_.output->appendPlainText("Skipping sample with SE ID '" + psn_id + "': no SAP ID available!");
				continue;
			}
			sap2psn[sap_id] = psn_id;

			NGSD mvh_db(true, "mvh");
			QString mvh_id = mvh_db.getValue("SELECT id FROM case_data WHERE sap_id='"+sap_id+"'").toString();
			if (mvh_id.isEmpty())
			{
				ui_.output->appendPlainText("Skipping sample with SE ID '" + psn_id + "': no entry in MVH case_data table for SAP id '"+sap_id+"'!");
				continue;
			}

			SqlQuery query = mvh_db.getQuery();
			query.prepare("UPDATE case_data SET se_id=:0, se_data=:1 WHERE id=:2");
			query.bindValue(0, psn_id);
			query.bindValue(1, "<items>\n"+psn2items[psn_id].join("\n")+"\n</items>");
			query.bindValue(2, mvh_id);
			query.exec();
		}


		//update main table
		int c_sap_id = GUIHelper::columnIndex(ui_.table, "SAP ID");
		int c_network_id = GUIHelper::columnIndex(ui_.table, "Netzwerk ID");
		for(int r=0; r<ui_.table->rowCount(); ++r)
		{
			QString sap_id = getString(r, c_sap_id);
			if (sap_id=="") continue;

			ui_.table->setItem(r, c_network_id, GUIHelper::createTableItem(sap2psn[sap_id]));
		}
	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());
	}
}

//TODO:
//- store variant data in SE RedCap database
//- trigger GRZ/KDK upload (Bericht geschrieben und Kündigung der Teilnahmeerklärung nicht vorhanden)

