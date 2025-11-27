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
#include <QFileDialog>
#include <QDesktopServices>
#include <QInputDialog>
#include "ExportHistoryDialog.h"
#include <QClipboard>
#include <QStandardPaths>

MVHub::MVHub(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, delayed_init_(this, true)
{
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableContextMenu(QPoint)));
	connect(ui_.table, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(openExportHistory(int)));
	connect(ui_.f_text, SIGNAL(textChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_cmid, SIGNAL(textChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_exclude_ps, SIGNAL(textChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_network, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_status, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_messages, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.f_ready_export, SIGNAL(stateChanged(int)), this, SLOT(updateTableFilters()));
	connect(ui_.f_export, SIGNAL(currentTextChanged(QString)), this, SLOT(updateTableFilters()));
	connect(ui_.export_consent_data, SIGNAL(clicked()), this, SLOT(exportConsentData()));
	connect(ui_.check_xml, SIGNAL(clicked()), this, SLOT(checkXML()));
}

void MVHub::delayedInitialization()
{
	//clear GUI and datastructures
	ui_.table->setRowCount(0);
	ui_.output->clear();
	cmid2messages_.clear();

	//load main data
	loadDataFromCM();
	loadDataFromSE();
	loadConsentData();

	determineProcessedSamples(0);
	updateExportStatus();

	//add missing data to SE RedCap
	int added_hpos = updateHpoTerms(0);
	int added_variants = updateVariants(0);

	//update SE RedCap data when we have changed it
	if (added_hpos>0 || added_variants>0)
	{
		loadDataFromSE();
	}

	checkForMetaDataErrors();
	showMessages();

	addOutputLine("done");

	//resize columns
	GUIHelper::resizeTableCellWidths(ui_.table, 200, -1);
	ui_.table->resizeRowsToContents();
	ui_.table->setColumnWidth(colOf("Netzwerk"), 300);
	ui_.table->setColumnWidth(colOf("export status"), 250);
}

void MVHub::tableContextMenu(QPoint pos)
{
	QList<int> rows = GUIHelper::selectedTableRows(ui_.table);

	//init
	int c_export_status = colOf("export status");

	//create menu
	QMenu menu;
	QAction* a_copy = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all");
	QAction* a_copy_sel = menu.addAction(QIcon(":/Icons/CopyClipboard.png"), "Copy selection");
	QMenu* copy_col_menu = menu.addMenu("Copy column");
	copy_col_menu->setIcon(QIcon(":/Icons/CopyClipboard.png"));
	copy_col_menu->addAction("CM ID");
	copy_col_menu->addAction("SAP ID");
	copy_col_menu->addAction("Netzwerk ID");
	copy_col_menu->addSeparator();
	copy_col_menu->addAction("PS");
	copy_col_menu->addAction("PS tumor");
	menu.addSeparator();
	QAction* a_show_ngsd = menu.addAction("Show all NGSD samples");
	a_show_ngsd->setEnabled(rows.count()==1);
	menu.addSeparator();
	QAction* a_export = menu.addAction("Export to GRZ/KDK");
	a_export->setEnabled(ui_.f_ready_export->isChecked() && rows.count()>0);
	QAction* a_export_history = menu.addAction("Show export history");
	a_export_history->setEnabled(rows.count()==1);
	QAction* a_export_failed_email = menu.addAction("Email: export failed");
	a_export_failed_email->setEnabled(rows.count()==1 && getString(rows.first(), c_export_status).contains("failed"));
	QAction* a_kdk_tan = menu.addAction("Copy KDK TAN");
	a_kdk_tan->setEnabled(rows.count()==1 && getNetwork(rows.first())==FBREK);

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

		int c_sap = colOf("SAP ID");
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
			NGSD mvh_db(true, "mvh");
			int c_cm = colOf("CM ID");
			int c_case_id = colOf("CM Fallnummer");
			int c_status = colOf("CM Status");
			int c_seq_type = colOf("Sequenzierungsart");

			foreach(int r, rows)
			{
				//init
				QString cm_id = getString(r, c_cm);
				QString title = "GRZ/KDK export of "+cm_id;

				//get ID
				QByteArray id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+cm_id+"'", false).toByteArray().trimmed();
				QByteArray case_id = getString(r, c_case_id).toLatin1().trimmed();

				//check if that patient did not get sequencing
				bool no_seq = getString(r, c_status)=="Abgebrochen" && getString(r, c_seq_type)=="Keine";

				//add GRZ export - if it is not pending/done
				if (!no_seq)
				{
					int pending_grz = mvh_db.getValue("SELECT count(*) FROM submission_grz WHERE case_id='"+id+"' and status='pending'").toInt();
					int done_grz = mvh_db.getValue("SELECT count(*) FROM submission_grz WHERE case_id='"+id+"' and status='done'").toInt();
					if (pending_grz>0)
					{
						//QMessageBox::warning(this, title, "GRZ export is pending. Be patient...");
					}
					else if (done_grz>0)
					{
						QMessageBox::warning(this, title, "GRZ export is already done!"); //TODO implement followup/addition/correction
					}
					else
					{
						QByteArray tag = case_id + "_" + QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1();
						QString tan = getTAN(tag, "GRZ");
						QString pseudo = getTAN(case_id, "GRZ", true);
						mvh_db.getQuery().exec("INSERT INTO `submission_grz`(`case_id`, `date`, `type`, `tang`, `pseudog`, `status`) VALUES ("+id+",CURDATE(),'initial','"+tan+"','"+pseudo+"','pending')");
						updateExportStatus(mvh_db, r);
					}
				}

				//add KDK export - if it is not pending/done
				if (getNetwork(r)==SE)
				{
					int pending_kdk = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='"+id+"' and status='pending'").toInt();
					int done_kdk = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='"+id+"' and status='done'").toInt();
					if (pending_kdk>0)
					{
						//QMessageBox::warning(this, title, "KDK export is pending. Be patient...");
					}
					else if (done_kdk>0)
					{
						QMessageBox::warning(this, title, "KDK export is already done!"); //TODO implement followup/addition/correction
					}
					else
					{
						QByteArray tag = case_id + "_" + QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1();
						QString tan = getTAN(tag, "KDK_SE");
						QString pseudo = getTAN(case_id, "KDK_SE", true);
						mvh_db.getQuery().exec("INSERT INTO `submission_kdk_se`(`case_id`, `date`, `type`, `tank`, `pseudok`, `status`) VALUES ("+id+",CURDATE(),'initial','"+tan+"','"+pseudo+"','pending')");
						updateExportStatus(mvh_db, r);
					}
				}

				//add KDK export as done (we just need the TAN)
				if (getNetwork(r)==FBREK)
				{
					int done_kdk = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='"+id+"' and status='done'").toInt();
					if (done_kdk>0)
					{
						QMessageBox::warning(this, title, "KDK export is already done!");
					}
					else
					{
						QByteArray tag = case_id + "_" + QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1();
						QString tan = getTAN(tag, "KDK_SE");
						mvh_db.getQuery().exec("INSERT INTO `submission_kdk_se`(`case_id`, `date`, `type`, `tank`, `pseudok`, `status`, `submission_output`) VALUES ("+id+",CURDATE(),'initial','"+tan+"','','done','Upload manually via HerediCare')");
						updateExportStatus(mvh_db, r);
					}
				}
			}
		}
		catch (Exception& e)
		{
			addOutputLine("Export error:\n"+e.message());
		}
	}
	if (action==a_export_history)
	{
		int r = rows.first();
		openExportHistory(r);
	}
	if (action==a_export_failed_email)
	{
		int r = rows.first();
		emailExportFailed(r);
	}
	if (action==a_kdk_tan)
	{
		int r = rows.first();
		int c_cm = colOf("CM ID");

		NGSD mvh_db(true, "mvh");
		QByteArray cm_id_db = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+getString(r, c_cm)+"'", false).toByteArray().trimmed();
		QStringList tans = mvh_db.getValues("SELECT tank FROM submission_kdk_se WHERE case_id='"+cm_id_db+"'");
		if (tans.count()==1)
		{
			QApplication::clipboard()->setText(tans.first());
		}
		else
		{
			QApplication::clipboard()->setText("");
		}
	}
	if (action->parent()==copy_col_menu)
	{
		QStringList output;
		int c = colOf(action->text());
		foreach(int r, rows)
		{
			QString entry = getString(r, c);
			if (entry.isEmpty()) continue;
			output << entry;
		}
		QApplication::clipboard()->setText(output.join("\n"));
	}
}

void MVHub::openExportHistory(int row)
{
	int c_cm = colOf("CM ID");
	int c_network = colOf("Netzwerk");

	ExportHistoryDialog dlg(this, getString(row, c_cm), getString(row, c_network));
	dlg.exec();
}

void MVHub::emailExportFailed(int row)
{
	//init
	QString cm_id = getString(row, colOf("CM ID"));
	QString sap_id = getString(row, colOf("SAP ID"));
	QString network_id = getString(row, colOf("Netzwerk ID"));
	NGSD mvh_db(true, "mvh");
	QString id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+cm_id+"'", false).toString().trimmed();

	//to email adress
	QString to;
	Network network = getNetwork(row);
	if (network==SE) to = Settings::string("email_se");
	if (network==OE) to = Settings::string("email_oe");
	if (network==FBREK) to = Settings::string("email_fbrek");

	//subject
	QString subject = "MVH Export fehlgeschlagen bei " + cm_id + (network_id.isEmpty() ? "" : " / " + network_id);

	//body
	QStringList body;
	body << "Hallo,";
	body << "";
	body << "bei folgendem Fall ist der Export fehlgeschlagen:";
	body << ("ID RedCap Fallverwaltung: " + cm_id);
	body << ("ID RedCap Netzwerk: " + network_id);
	body << ("ID SAP: " + sap_id);
	body << "";
	body << "Fehler:";

	//GRZ
	SqlQuery query = mvh_db.getQuery();
	query.exec("SELECT * FROM submission_grz WHERE case_id='" + id + "' ORDER BY id DESC LIMIT 1");
	if(query.next() && query.value("status")=="failed")
	{
		QStringList lines = query.value("submission_output").toString().split("\n");
		foreach(QString line, lines)
		{
			if (!line.contains("error", Qt::CaseInsensitive)) continue;
			body << ("GRZ: " + line);
		}
	}

	//KDK SE
	query.exec("SELECT * FROM submission_kdk_se WHERE case_id='" + id + "' ORDER BY id DESC LIMIT 1");
	if(query.next() && query.value("status")=="failed")
	{
		QStringList lines = query.value("submission_output").toString().split("\n");
		foreach(QString line, lines)
		{
			if (!line.contains("error", Qt::CaseInsensitive)) continue;
			body << ("KDK: " + line);
		}
	}

	body << "";
	body << "Viele Grüße,";
	body << "  " + Settings::string("email_sender_name");

	//open in email app
	QDesktopServices::openUrl(QUrl("mailto:" + to + "?subject=" + subject + "&body=" + body.join("\n")));
}

void MVHub::updateTableFilters()
{
	const int rows = ui_.table->rowCount();
	const int cols = ui_.table->columnCount();
	QBitArray visible(rows, true);

	//apply CM-ID filter
	QString f_cmid = ui_.f_cmid->text().trimmed();
	if (!f_cmid.isEmpty())
	{
		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			if (getString(r, 0)!=f_cmid)
			{
				visible[r] = false;
			}
		}
	}

	//apply status filter
	QString f_status = ui_.f_status->currentText();
	if (!f_status.isEmpty())
	{
		int c_status = colOf("CM Status");
		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			visible[r] = getString(r, c_status)==f_status;
		}
	}

	//apply network filter
	QString f_network = ui_.f_network->currentText();
	if (!f_network.isEmpty())
	{
		int c_network = colOf("Netzwerk");
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
	if (ui_.f_messages->currentText()!="")
	{
		int c_messages = colOf("messages");
		QString messages_filter = ui_.f_messages->currentText();

		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			QString messages = getString(r, c_messages).trimmed();
			if (messages_filter=="exist" && messages.isEmpty())
			{
				visible[r] = false;
			}
			if (messages_filter=="none" && !messages.isEmpty())
			{
				visible[r] = false;
			}
		}
	}

	//export status filter
	if (ui_.f_export->currentText()!="")
	{
		int c_exp_status = colOf("export status");
		int c_exp_conf = colOf("export confirmation");
		int c_case_status = colOf("CM Status");
		QString export_filter = ui_.f_export->currentText();
		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			QString status = getString(r, c_exp_status);
			int c_todo = status.split("//").count();
			QString conf = getString(r, c_exp_conf);
			int c_conf = conf.isEmpty() ? 0 : conf.split("//").count();

			if (export_filter=="pending" && status!="")
			{
				visible[r] = false;
			}
			if (export_filter=="pending" && getString(r, c_case_status)=="Abgebrochen" && getNetwork(r)!=SE)
			{
				visible[r] = false;
			}

			if (export_filter=="in progress" && !(status!="" && c_todo!=c_conf))
			{
				visible[r] = false;
			}

			if (export_filter=="done" && !(status!="" && c_todo==c_conf))
			{
				visible[r] = false;
			}
		}
	}

	//apply GRZ/KDK export filter
	if (ui_.f_ready_export->isChecked())
	{
		int c_case_id = colOf("CM Fallnummer");
		int c_case_status = colOf("CM Status");
		int c_seq_type = colOf("Sequenzierungsart");
		int c_network_id = colOf("Netzwerk ID");
		int c_consent = colOf("consent");
		int c_consent_cm = colOf("consent signed [CM]");
		int c_consent_ver = colOf("consent version [SE]");
		int c_report_date = colOf("Befunddatum");
		int c_te_retracted = colOf("Kündigung TE");
		int c_ps_n = colOf("PS");
		int c_ps_t = colOf("PS tumor");

		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			Network network = getNetwork(r);
			if (network==SE)
			{
				//done
				QString status = getString(r, c_case_status);
				if (status!="Abgeschlossen" && status!="Abgebrochen")
				{
					visible[r] = false;
					continue;
				}

				//base data available
				if (getString(r, c_case_id)=="" || getString(r, c_network_id)=="")
				{
					visible[r] = false;
					continue;
				}

				//consent data available
				if (getString(r, c_consent_ver).startsWith("Erwachsene") && getString(r, c_consent)=="" && getString(r, c_consent_cm)!="Nein")
				{
					visible[r] = false;
					continue;
				}

				//TE not retracted
				if (getString(r, c_te_retracted)!="")
				{
					visible[r] = false;
					continue;
				}

				//Abgeschlossen, but no report date set
				if (status=="Abgeschlossen" && getString(r, c_report_date)=="")
				{
					visible[r] = false;
					continue;
				}

				//Abgebrochen, seqencing type not 'keine'
				if (status=="Abgebrochen" && getString(r, c_seq_type)!="Keine")
				{
					visible[r] = false;
					continue;
				}
			}
			else if (network==FBREK)
			{
				//done
				QString status = getString(r, c_case_status);
				if (status!="Abgeschlossen" && status!="Abgebrochen" && status!="Follow-Up")
				{
					visible[r] = false;
					continue;
				}

				//base data available
				if (getString(r, c_case_id)=="")
				{
					visible[r] = false;
					continue;
				}

				//consent signed info available
				if (getString(r, c_consent_cm)=="")
				{
					visible[r] = false;
					continue;
				}

				//TE not retracted
				if (getString(r, c_te_retracted)!="")
				{
					visible[r] = false;
					continue;
				}

				//Abgeschlossen, but no report date set
				if (status=="Abgeschlossen" && getString(r, c_report_date)=="")
				{
					visible[r] = false;
					continue;
				}

				//Abgebrochen, seqencing type not 'keine'
				if (status=="Abgebrochen" && getString(r, c_seq_type)!="Keine")
				{
					visible[r] = false;
					continue;
				}
			}
			else if (network==OE)
			{
				//done
				QString status = getString(r, c_case_status);
				if (status!="Abgeschlossen" && status!="Abgebrochen" && status!="Follow-Up")
				{
					visible[r] = false;
					continue;
				}

				//documentation incomplete
				if (ui_.table->item(r,2)->toolTip()!="")
				{
					visible[r] = false;
					continue;
				}

				//base data available
				if (getString(r, c_case_id)=="")
				{
					visible[r] = false;
					continue;
				}

				//consent signed info available
				if (getString(r, c_consent_cm)=="")
				{
					visible[r] = false;
					continue;
				}

				//TE not retracted
				if (getString(r, c_te_retracted)!="")
				{
					visible[r] = false;
					continue;
				}

				//seqencing type valid
				if (status=="Abgebrochen" && getString(r, c_seq_type)!="Keine")
				{
					visible[r] = false;
					continue;
				}
				if (status!="Abgebrochen" && (getString(r, c_seq_type)=="Keine" || getString(r, c_seq_type)==""))
				{
					visible[r] = false;
					continue;
				}

				//tumor/normal PS found
				if (getString(r, c_ps_t)=="" || getString(r, c_ps_n)=="")
				{
					visible[r] = false;
					continue;
				}
			}
			else //TODO implement for other networks
			{
				visible[r] = false;
				continue;
			}
		}
	}

	//apply exclude PS filter
	QString f_exclude_ps = ui_.f_exclude_ps->text().replace(',', ' ').simplified();
	if (!f_exclude_ps.isEmpty())
	{
		int c_ps = colOf("PS");
		int c_ps_tumor = colOf("PS tumor");

		QSet<QString> exclude = LIST_TO_SET(f_exclude_ps.split(' '));
		for (int r=0; r<rows; ++r)
		{
			if (!visible[r]) continue;

			if (exclude.contains(getString(r, c_ps)) || exclude.contains(getString(r, c_ps_tumor))) visible[r] = false;
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
	addOutputHeader("loading consent data", false);

	int c_cm = colOf("CM ID");
	int c_sap = colOf("SAP ID");
	int c_consent = colOf("consent");
	if (ui_.table->columnWidth(c_consent)<300) ui_.table->setColumnWidth(c_consent, 300);

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString cm_id = getString(r, c_cm);
		QString sap_id = getString(r, c_sap);
		QByteArray consent_json = getConsent(sap_id);
		QString consent_xml = consentJsonToXml(consent_json);

		//store consent data in MVH database
		NGSD mvh_db(true, "mvh");
		SqlQuery query = mvh_db.getQuery();
		query.prepare("UPDATE case_data SET rc_data=:0, rc_data_json=:1 WHERE cm_id=:2");
		query.bindValue(0, consent_xml);
		query.bindValue(1, consent_json);
		query.bindValue(2, cm_id);
		query.exec();


		QTableWidgetItem* item = GUIHelper::createTableItem(consent_xml.isEmpty() ? "" : "yes (see tooltip)");
		item->setToolTip(consent_xml);
		ui_.table->setItem(r, c_consent, item);
	}
}

void MVHub::exportConsentData()
{
	QString title = "Export broad consent JSON files using the meDIC API.";

	//get file names
	QString text = QInputDialog::getMultiLineText(this, title, "SAP IDs (one per line or first column in TSV):");
	if(text.isEmpty()) return;

	//store output
	QString folder = QFileDialog::getExistingDirectory(this, "Folder to store consent JSON files", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
	if(folder.isEmpty()) return;

	//store output files
	foreach(QString line, text.split('\n'))
	{
		QStringList parts = line.split('\t');
		QString sap_id = parts[0];
		Helper::storeTextFile(folder + "/broad_consent_" + sap_id + ".json", QStringList() << getConsent(sap_id));
	}
}

void MVHub::checkXML()
{
	addOutputHeader("checking XML data");

	//init
	NGSD mvh_db(true, "mvh");
	QString tmp_file = Helper::tempFileName(".xml");

	//iterate over all rows
	SqlQuery query = mvh_db.getQuery();
	query.exec("SELECT id, cm_data, se_data, rc_data FROM case_data");
	while(query.next())
	{
		QString id = query.value("id").toString();

		//iterate over all fields
		foreach(QString field, QStringList() << "cm_data" << "se_data" << "rc_data")
		{
			QString data = query.value(field).toString().trimmed();
			if (data.isEmpty()) continue;

			try
			{
				Helper::storeTextFile(tmp_file, QStringList() << "<?xml version=\"1.0\" encoding=\"iso-8859-1\" ?>" << data.split("\n"));
				QString error = XmlHelper::isValidXml(tmp_file);
				if (!error.isEmpty())
				{
					addOutputLine(id +"\t" + field + "\t" + error);
				}
			}
			catch (Exception& e)
			{
				addOutputLine(id +"\t" + field + "\t" + e.message());
			}
		}
	}

	addOutputLine("");
	addOutputLine("done");
}

void MVHub::on_actionReloadData_triggered()
{
	//reload data
	delayedInitialization();

	//apply filters
	updateTableFilters();
}


void MVHub::on_actionReloadExportStatus_triggered()
{
	ui_.output->clear();

	updateExportStatus();

	addOutputLine("done");

	//apply filters
	updateTableFilters();
}

void MVHub::on_actionAbout_triggered()
{
	QString about_text = QApplication::applicationName() + " " + QCoreApplication::applicationVersion();

	QMessageBox::about(this, "About " + QApplication::applicationName(), about_text);
}

void MVHub::determineProcessedSamples(int debug_level)
{
	addOutputHeader("determining processed samples (if missing)", false);

	//init
	NGSD db;
	GenLabDB genlab;
	NGSD mvh_db(true, "mvh");
	int c_cm = colOf("CM ID");
	int c_sap = colOf("SAP ID");
	int c_seq_type = colOf("Sequenzierungsart");
	int c_ps = colOf("PS");
	int c_ps_t = colOf("PS tumor");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		//skip if no network is set
		Network network = getNetwork(r);
		if (network==UNSET)
		{
			if (debug_level>=1) addOutputLine("  skipped: No network");
			continue;
		}

		//skip if already in MVH database
		QString cm_id = getString(r, c_cm);
		QString sap_id = getString(r, c_sap);
		if (debug_level>=1) addOutputLine("cm_id: " + cm_id + " // SAP ID: " + sap_id);
		QString ps_mvh = mvh_db.getValue("SELECT ps FROM case_data WHERE cm_id='"+cm_id+"'").toString().trimmed();
		QString ps_mvh_t = mvh_db.getValue("SELECT ps_t FROM case_data WHERE cm_id='"+cm_id+"'").toString().trimmed();
		if(network==SE || network==FBREK)
		{
			if (!ps_mvh.isEmpty())
			{
				ui_.table->setItem(r, c_ps, GUIHelper::createTableItem(ps_mvh));
				if (debug_level>=1) addOutputLine("  skipped: PS already in MV db (SE)");
				continue;
			}
		}
		else if(network==OE)
		{
			if (!ps_mvh.isEmpty() && !ps_mvh_t.isEmpty())
			{
				ui_.table->setItem(r, c_ps, GUIHelper::createTableItem(ps_mvh));
				ui_.table->setItem(r, c_ps_t, GUIHelper::createTableItem(ps_mvh_t));
				if (debug_level>=1) addOutputLine("  skipped: PS already in MV db (SE)");
				continue;
			}
		}
		else THROW(ProgrammingException, "Unhandled network type '" + networkToString(network) + "'");

		//skip if no sequencing was done
		QString seq_type = getString(r, c_seq_type);
		if (seq_type.isEmpty() || seq_type=="Keine")
		{
			if (debug_level>=1) addOutputLine("  skipped: No sequencing type or sequencing type is 'Keine'");
			continue;
		}

		//determine search parameters
		ProcessedSampleSearchParameters params;
		params.run_finished = true;
		params.p_type = "diagnostic";
		params.r_after = QDate(2024, 7, 1);
		bool post_filter_wgs_lrgs = false;
		if ((network==SE || network==FBREK) && seq_type=="WGS")
		{
			params.include_bad_quality_samples = false;
			params.include_tumor_samples = false;
			post_filter_wgs_lrgs = true; //system type can be WGS or lrGS > filtering below
		}
		else if (network==OE && seq_type=="WES")
		{
			params.include_bad_quality_samples = true;
			params.include_tumor_samples = true;
			params.sys_type = "WES";
		}
		else if (network==OE && seq_type=="WGS")
		{
			params.include_bad_quality_samples = true;
			params.include_tumor_samples = true;
			params.sys_type = "WGS";
		}
		else
		{
			cmid2messages_[cm_id] << "Could not determine processed sample(s): unhandled combination of network '"+networkToString(network)+"' and sequencing type '"+seq_type+"'";
		}

		//perform search and determine possible PS
		QStringList ps_list_germline;
		QStringList ps_list_tumor;
		QStringList tmp = genlab.samplesWithSapID(sap_id, params);
		foreach(QString ps, tmp)
		{
			QString ps_id = db.processedSampleId(ps);
			QString s_id = db.sampleId(ps);
			if (post_filter_wgs_lrgs)
			{
				ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
				QString sys_type = ps_data.processing_system_type;
				if (sys_type!="WGS" && sys_type!="lrGS") continue; //system type can be WGS or lrGS > filtering here instead via parameters
			}


			SampleData s_data = db.getSampleData(s_id);
			if (!s_data.is_tumor)
			{
				ps_list_germline << ps;
			}
			else
			{
				ps_list_tumor << ps;
			}
		}

		//set germline PS if exactly one PS found
		if (ps_list_germline.count()==0)
		{
			cmid2messages_[cm_id] << "Could not determine processed sample(s): no matching samples in NGSD";
		}
		else if (ps_list_germline.count()>1)
		{
			cmid2messages_[cm_id] << "Could not determine processed sample(s): several matching samples in NGSD";
		}
		else
		{
			QString ps = ps_list_germline.first();
			ui_.table->setItem(r, c_ps, GUIHelper::createTableItem(ps));

			//store PS in MVH database
			SqlQuery query = mvh_db.getQuery();
			query.exec("UPDATE case_data SET ps='"+ps+"' WHERE cm_id='"+cm_id+"'");
		}

		//set tumor PS if exactly one PS found
		if (network==OE)
		{
			if (ps_list_tumor.count()==0)
			{
				cmid2messages_[cm_id] << "Could not determine tumor processed sample(s): no matches in NGSD";
			}
			else if (ps_list_tumor.count()>1)
			{
				cmid2messages_[cm_id] << "Could not determine tumor processed sample(s): several matches in NGSD";
			}
			else
			{
				QString ps = ps_list_tumor.first();
				ui_.table->setItem(r, c_ps_t, GUIHelper::createTableItem(ps));

				//store PS in MVH database
				SqlQuery query = mvh_db.getQuery();
				query.exec("UPDATE case_data SET ps_t='"+ps+"' WHERE cm_id='"+cm_id+"'");
			}
		}
	}
}

int MVHub::updateHpoTerms(int debug_level)
{
	int added_hpo_terms = 0;
	addOutputHeader("updating HPO terms in SE RedCap", false);

	//init
	NGSD db;
	NGSD mvh_db(true, "mvh");
	int c_se_id = colOf("Netzwerk ID");
	int c_ps = colOf("PS");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		//no SE ID => skip
		QString se_id = getString(r, c_se_id);
		if (se_id.isEmpty()) continue;

		//no PS => skip
		QString ps = getString(r, c_ps);
		if (ps.isEmpty()) continue;

		//only for SE
		if (getNetwork(r)!=SE) continue;

		//determine HPO terms in MVH database
		PhenotypeList hpo_mvh;
        QByteArray se_data = mvh_db.getValue("SELECT se_data FROM case_data WHERE se_id='"+se_id+"'").toByteArray();
        QDomDocument doc;
        try
        {
            doc = XmlHelper::parse(se_data);
        }
        catch (Exception& e)
        {
            addOutputLine(se_id + "/" + ps + ": could not parse SE data: " + e.message());
            continue;
        }
		QDomElement root = doc.documentElement();
        for(QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			QDomElement e = n.toElement(); // try to convert the node to an element.
			if(!e.isNull() && e.nodeName()=="item")
			{
				QDomElement e2 = e.namedItem("hpo").toElement();
				if(e2.isNull()) continue;

				QByteArray hpo_name = e2.text().trimmed().toUtf8();
				if (hpo_name.isEmpty()) continue;

				//there is no easy way to get IDs, thus we have to handle terms with changed names...
				if (hpo_name=="Leukopenia") hpo_name = "Decreased total leukocyte count"; //renamed
				if (hpo_name=="Monocytopenia") hpo_name = "Decreased total monocyte count"; //renamed
				if (hpo_name=="Eosinophilia") hpo_name = "Increased total eosinophil count"; //renamed
				if (hpo_name=="Poor motor coordination") hpo_name = "Incoordination"; //obsolete and replaced

				//handle terms with accession instead of name (RedCap bug in Ontology handling)
				if (hpo_name.startsWith("HP:"))
				{
					hpo_name = db.getValue("SELECT name FROM hpo_term WHERE hpo_id='"+hpo_name+"'").toByteArray();
				}

				int hpo_db_id = db.phenotypeIdByName(hpo_name, false);
				if (hpo_db_id!=-1)
				{
					hpo_mvh << db.phenotype(hpo_db_id);
				}
				else
				{
					addOutputLine(se_id + "/" + ps + ": invalid HPO term with name '" + hpo_name + "' in SE RedCap!");
				}
			}
		}

		//determine HPO terms in NGSD
		QString s_id = db.sampleId(ps);
		PhenotypeList hpo_ngsd = db.samplePhenotypes(s_id, false);

		if (debug_level>=2) addOutputLine(se_id + "/" + ps + ": HPOs NGSD: " + QString::number(hpo_ngsd.count()) + " HPOs SE RedCap: " + QString::number(hpo_mvh.count()));

		//update if terms are missing
        for(const Phenotype& hpo: hpo_ngsd)
		{
			if (hpo_mvh.containsAccession(hpo.accession()))
			{
				if (debug_level>=3) addOutputLine(se_id + "/" + ps + ": HPO skipped (already in SE RedCap): " + hpo.toString());
				continue;
			}

			if (debug_level>=1) addOutputLine(se_id + "/" + ps + ": adding HPO: " + hpo.toString());

			//TODO replace fixed version_hpo with correct value from NGSD when implemented: https://github.com/imgag/megSAP/issues/423
			//add HPO term to SE RedCap
			addOutputLine(se_id + "/" + ps + ": adding HPO term "+hpo.toString()+" ...");
			HttpHeaders headers;
			headers.insert("Content-Type", "application/x-www-form-urlencoded");
			QByteArray xml =   "<records>"
								"	<item>"
								"		<psn><![CDATA[" + se_id.toUtf8() + "]]></psn>"
								"		<redcap_repeat_instrument><![CDATA[hpo]]></redcap_repeat_instrument>"
								"		<redcap_repeat_instance><![CDATA[new]]></redcap_repeat_instance>"
								"		<hpo><![CDATA[" + hpo.accession() + "]]></hpo>"
								"		<version_hpo><![CDATA[2025-03-03]]></version_hpo>"
								"		<beginn_symptome_nb___000_01><![CDATA[1]]></beginn_symptome_nb___000_01>"
								"		<hpo_complete><![CDATA[2]]></hpo_complete>"
								"	</item>"
								"</records>";
			QByteArray data = "token="+Settings::string("redcap_se_token").toLatin1()+"&content=record&format=xml&type=flat&data="+xml;
			HttpHandler handler(true);
			QByteArray reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers);
			if (!reply.contains("<count>1</count>"))
			{
				addOutputLine(se_id + "/" + ps + ": ERROR creating HPO term in SE RedCap! Reply:\n" +reply);
				return added_hpo_terms;
			}
			++added_hpo_terms;
		}
	}

	addOutputLine("added " + QString::number(added_hpo_terms) + " HPO terms");

	return added_hpo_terms;
}

int MVHub::updateVariants(int debug_level)
{
	int added_variants = 0;
	addOutputHeader("updating variants in SE RedCap", false);

	//init
	NGSD db;
	NGSD mvh_db(true, "mvh");
	int c_se_id = colOf("Netzwerk ID");
	int c_ps = colOf("PS");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		//no SE ID => skip
		QString se_id = getString(r, c_se_id);
		if (se_id.isEmpty()) continue;

		//no PS => skip
		QString ps = getString(r, c_ps);
		if (ps.isEmpty()) continue;

		//only for SE
		if (getNetwork(r)!=SE) continue;

		//determine variants in MVH database
		QStringList variants_mvh;
        QByteArray se_data = mvh_db.getValue("SELECT se_data FROM case_data WHERE se_id='"+se_id+"'").toByteArray();
        QDomDocument doc;
        try
        {
            doc = XmlHelper::parse(se_data);
        }
        catch (Exception& e)
        {
            addOutputLine(se_id + "/" + ps + ": could not parse SE data: " + e.message());
            continue;
        }
		QDomElement root = doc.documentElement();
		for(QDomNode n = root.firstChild(); !n.isNull(); n = n.nextSibling())
		{
			QDomElement e = n.toElement(); // try to convert the node to an element.
			if(!e.isNull() && e.nodeName()=="item")
			{
				QDomElement e2 = e.namedItem("variante").toElement();
				if(e2.isNull()) continue;

				QByteArray var = e2.text().trimmed().toUtf8();
				if (var.isEmpty()) continue;
				variants_mvh << var;
			}
		}

		//determine variants terms in NGSD
		QList<VarData> variants_ngsd = getVariants(db, ps);

		if (debug_level>=3) addOutputLine(se_id + "/" + ps + ": variants in NGSD: " + QString::number(variants_ngsd.count()) + " // variants SE RedCap: " + QString::number(variants_mvh.count()));

		//add missing variants to SE RedCap
		foreach(VarData var, variants_ngsd)
		{
			if (variants_mvh.contains(var.name))
			{
				if (debug_level>=2) addOutputLine(se_id + "/" + ps + ": variant skipped (already in SE RedCap): " + var.name);
				continue;
			}

			//determine variant type
			QByteArray var_typ;
			if (var.type==VarType::CAUSAL) var_typ = "1";
			else if (var.type==VarType::INCIDENTAL) var_typ = "2";
			else if (var.type==VarType::VUS) var_typ = "3";
			else THROW(ProgrammingException, "Inhandled variant type '" + QString::number(var.type) + "'!");

			//add variant to SE RedCap
			if (debug_level>=1) addOutputLine(se_id + "/" + ps + ": adding variant "+var.name+" ...");
			HttpHeaders headers;
			headers.insert("Content-Type", "application/x-www-form-urlencoded");
			QByteArray xml =   "<records>"
								"	<item>"
								"		<psn><![CDATA[" + se_id.toUtf8() + "]]></psn>"
								"		<redcap_repeat_instrument><![CDATA[varianten]]></redcap_repeat_instrument>"
								"		<redcap_repeat_instance><![CDATA[new]]></redcap_repeat_instance>"
								"		<variante><![CDATA[" + var.name + "]]></variante>"
								"		<lok_variante><![CDATA[" + var.localization + "]]></lok_variante>"
								"		<typ_variante><![CDATA[" + var_typ + "]]></typ_variante>"
								"		<varianten_complete><![CDATA[2]]></varianten_complete>"
								"	</item>"
								"</records>";
			QByteArray data = "token="+Settings::string("redcap_se_token").toLatin1()+"&content=record&format=xml&type=flat&data="+xml;
			HttpHandler handler(true);
			QByteArray reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers);
			if (!reply.contains("<count>1</count>"))
			{
				addOutputLine(se_id + "/" + ps + ": ERROR creating variant in SE RedCap! Reply:\n" +reply);
				return added_variants;
			}
			++added_variants;
		}
	}

	addOutputLine("added " + QString::number(added_variants) + " variants");

	return added_variants;
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
	int c_cm = colOf("CM ID");
	int c_export_status = colOf("export status");

	//get ID
	QString cm_id = getString(r, c_cm);
	QString id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+cm_id+"'", true).toString().trimmed();
	if (id.isEmpty()) return; //this can happen when the SAP ID is listed twice in SE RedCap

	QString text = "";

	//add status of latest GRZ upload
	SqlQuery query = mvh_db.getQuery();
	query.exec("SELECT * FROM submission_grz WHERE case_id='" + id + "' ORDER BY id DESC LIMIT 1");
	if(query.next())
	{
		QString status = query.value("status").toString();
		text += "GRZ " + (status=="done" ? query.value("date").toString() : status);
		if (status=="failed") text += QChar(0x274C);
		if (status=="done") text += QChar(0x2705);

		//add previous uploads
		int c_other = mvh_db.getValue("SELECT count(*) FROM submission_grz WHERE case_id='" + id + "' AND id!='"+query.value("id").toString()+"'").toInt();
		if (c_other>0) text += " +" + QString::number(c_other);
	}

	//add status of latest KDK upload
	query.exec("SELECT * FROM submission_kdk_se WHERE case_id='" + id + "' ORDER BY id DESC LIMIT 1");
	if(query.next())
	{
		QString status = query.value("status").toString();
		if (!text.isEmpty()) text += " // ";
		text += "KDK " + (status=="done" ? query.value("date").toString() : status);
		if (status=="failed") text += QChar(0x274C);
		if (status=="done") text += QChar(0x2705);

		//add previous uploads
		int c_other = mvh_db.getValue("SELECT count(*) FROM submission_kdk_se WHERE case_id='" + id + "' AND id!='"+query.value("id").toString()+"'").toInt();
		if (c_other>0) text += " +" + QString::number(c_other);
	}

	//add table item
	ui_.table->setItem(r, c_export_status, GUIHelper::createTableItem(text));
}

void MVHub::checkForMetaDataErrors()
{
	addOutputHeader("checking for meta data errors", false);

	NGSD db;
	int c_cm_id = colOf("CM ID");
	int c_cm_status = colOf("CM Status");
	int c_seq_type = colOf("Sequenzierungsart");
	int c_consent_cm = colOf("consent signed [CM]");
	int c_consent_ver = colOf("consent version [SE]");
	int c_consent = colOf("consent");
	int c_report_date = colOf("Befunddatum");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString cm_id = getString(r, c_cm_id);
		QString seq_type = getString(r, c_seq_type);
		QString status = getString(r, c_cm_status);

		//check for errors only if case is finished
		if (status!="Abgeschlossen" && status=="Abgebrochen" && status=="Follow-Up") continue;

		Network network = getNetwork(r);

		//check if docu is complete
		if (ui_.table->item(r,2)->toolTip()!="")
		{
			cmid2messages_[cm_id] << "CM docu not flagged as complete";
		}

		//consent meta data in CM RedCap missing
		QString bc_signed = getString(r, c_consent_cm);
		if (bc_signed=="")
		{
			cmid2messages_[cm_id] << "No info about BC in CM RedCap";
		}
		else if (bc_signed=="Ja")
		{
			if (network==SE)
			{
				QString consent_ver = getString(r, c_consent_ver);
				if (consent_ver=="")
				{
					cmid2messages_[cm_id] << "No BC type in SE RedCap";
				}
				else if (consent_ver.startsWith("Erwachsene"))
				{
					//consent missing
					if (getString(r, c_consent)=="")
					{
						cmid2messages_[cm_id] << "No consent data in SAP";
					}
				}
			}

			if (network==OE || network==FBREK)
			{
				//consent missing
				if (getString(r, c_consent)=="")
				{
					cmid2messages_[cm_id] << "No consent data in SAP";
				}
			}
		}

		//checks for status 'Abgeschlossen'
		if (status=="Abgeschlossen")
		{
			if (getString(r, c_report_date)=="")
			{
				cmid2messages_[cm_id] << "Status 'Abgeschlossen', but no report date set";
			}
			if (seq_type=="Keine" || seq_type=="")
			{
				cmid2messages_[cm_id] << "Status 'Abgeschlossen', but sequencing type not valid: '" + seq_type + "'";
			}
		}

		//checks for status 'Abgebrochen'
		if (status=="Abgebrochen")
		{
			if (seq_type!="Keine")
			{
				cmid2messages_[cm_id] << "Status 'Abgebrochen', but sequencing type is not 'Keine'";
			}
		}

		//checks for status 'Follow-Up' (is basically 'Abgeschlossen' in OE)
		if (network==OE && status=="Follow-Up")
		{
			if (seq_type=="Keine" || seq_type=="")
			{
				cmid2messages_[cm_id] << "Status 'Follow-Up', but sequencing type not valid: '" + seq_type + "'";
			}
		}

		//check germline sample is in study
		if (network!=OE) //for OE, only the tumor sample gets the study!
		{
			int c_ps = colOf("PS");
			QString ps = getString(r, c_ps).split(" ").first();
			if (ps!="")
			{
				QString ps_id = db.processedSampleId(ps);
				if (!db.studies(ps_id).contains("Modellvorhaben_2024"))
				{
					cmid2messages_[cm_id] << (ps +" not in study");
				}
			}
		}

		//check tumor sample is in study
		if (network==OE)
		{
			int c_ps_t = colOf("PS tumor");
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
}

void MVHub::showMessages()
{
	int c_cm = colOf("CM ID");
	int c_messages = colOf("messages");

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

		//addOutputLine("base64-encoded key: " +data.toBase64());
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
		addOutputLine("");
		addOutputLine("### " + section + " ###");
		addOutputLine("");
	}
}

void MVHub::addOutputLine(QString line)
{
	ui_.output->appendPlainText(line);
	ui_.output->ensureCursorVisible();
	qApp->processEvents();
}

QByteArray MVHub::getConsent(QString sap_id, bool debug)
{
	static QByteArray token = "";
	static QDateTime token_datetime = QDateTime(QDate(1970, 1, 1), QTime(0,0));
	try
	{
		//meDIC API expects SAP ID padded to 10 characters...
		sap_id = sap_id.trimmed();
		while (sap_id.size()<10) sap_id.prepend('0');

		//test or production
		bool test_server = false;
		if (test_server)
		{
			addOutputLine("ATTENTION: using test servers for getConsent");
			addOutputLine("");
		}

		//get token if missing or older than 5 minutes
		if (token.isEmpty() || token_datetime<QDateTime::currentDateTime().addSecs(-300))
		{
			if (debug)

			{
				addOutputLine("");
				addOutputLine("Getting consent token...");
			}
			HttpHeaders headers;
			headers.insert("Content-Type", "application/x-www-form-urlencoded");
			headers.insert("Prefer", "handling=strict");
			QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("consent_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("consent_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
			QString url = test_server ? "https://tc-t.med.uni-tuebingen.de/auth/realms/consent-test/protocol/openid-connect/token" : "https://tc-p.med.uni-tuebingen.de/auth/realms/consent/protocol/openid-connect/token";
			if (debug) addOutputLine("URL: "+url);
			if (debug) addOutputLine("data: " + data);
			HttpHandler handler(true);
			QByteArray reply = handler.post(url, data, headers);
			QByteArrayList parts = reply.split('"');
			if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

			token = parts[3];
			token_datetime = QDateTime::currentDateTime();
			if (debug) addOutputLine("Token: "+ token);
		}

		//get consent
		if (debug) addOutputLine("Getting consent for "+sap_id+"...");
		HttpHeaders headers2;
		headers2.insert("Authorization", "Bearer "+token);
		QString url = test_server ? "https://tc-t.med.uni-tuebingen.de:8443/fhir/Consent" : "https://tc-p.med.uni-tuebingen.de:8443/fhir/Consent";
		url += "?patient.identifier="+sap_id;
		HttpHandler handler(true);
		QByteArray reply = handler.get(url, headers2);
		if (debug)
		{
			addOutputLine("URL: " + url);
			addOutputLine("SAP ID: " + sap_id);
			addOutputLine("Consent: " + reply);
		}

		//filter consent (has to be active, v09 cannot be used for MVH)
		QJsonDocument doc = QJsonDocument::fromJson(reply);
		QJsonArray entries = doc.object()["entry"].toArray();
		QJsonArray entries_filtered;
		for (int i=0; i<entries.count(); ++i)
		{
			QJsonObject object = entries[i].toObject();
			if (!object.contains("resource")) continue;

			object = object["resource"].toObject();

			//check that the resource type is a consent
			if (!object.contains("resourceType")) continue;
			if (object["resourceType"].toString()!="Consent") continue;

			//check that the status type is active
			if (!object.contains("status")) continue;
			if (object["status"].toString()!="active")
			{
				if (debug) addOutputLine("  skipped BC because it is not active: " + object["status"].toString());
				continue;
			}

			//check that the consent version is not V9
			if (!object.contains("identifier")) continue;
			bool is_v9 = true;
			QJsonArray identifiers = object["identifier"].toArray();
			for (int i=0; i<identifiers.count(); ++i)
			{
				QJsonObject object = identifiers[i].toObject();
				if (!object.contains("system")) continue;
				if (object["system"].toString()!="source.ish.document.v09") is_v9 = false;
			}
			if (is_v9)
			{
				if (debug) addOutputLine("  skipped BC because it is V09");
				continue;
			}

			entries_filtered.append(entries[i]);
		}
		QJsonObject tmp = doc.object();
		tmp.insert("entry", entries_filtered);
		doc.setObject(tmp);

		return doc.toJson();
	}
	catch (Exception& e)
	{
		addOutputLine("ERROR:");
		addOutputLine(e.message());

		//clear token in case of error
		token = "";
	}

	return "";
}

QByteArray MVHub::consentJsonToXml(QByteArray json_text, bool debug)
{
	QByteArrayList output;

	QJsonDocument doc = QJsonDocument::fromJson(json_text);
	QJsonArray entries = doc.object()["entry"].toArray();
	for (int i=0; i<entries.count(); ++i)
	{
		QJsonObject object = entries[i].toObject();
		if (!object.contains("resource")) THROW(ArgumentException, "BC entry does not contain 'resource' key!");

		object = object["resource"].toObject();

		//check that the resource type is a consent
		if (!object.contains("resourceType")) THROW(ArgumentException, "BC entry does not contain 'resourceType' key!");
		if (object["resourceType"].toString()!="Consent") THROW(ArgumentException, "BC entry has 'resourceType' other than 'Consent'!");

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
		QDate date = QDate::fromString(object["dateTime"].toString().left(10), Qt::ISODate);
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
	if (debug)
	{
		addOutputLine("  consent XML:");
		addOutputLine(output.join("\n"));
	}
	return output.join("\n");
}

QByteArray MVHub::getTAN(QByteArray str, QByteArray context, bool skip_pseudo1, bool test_server, bool debug)
{
	//check context
	if (context!="GRZ" && context!="KDK_SE")
	{
		THROW(ProgrammingException, " MVHub::getPseudonym: unknown context '" + context + "'!");
	}

	//test or production
	if (test_server)
	{
		addOutputLine("ATTENTION: using test server!");
		addOutputLine("");
	}

	//get token
	QByteArray token = "";
	{
		if (debug) addOutputLine("Getting token...");

		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		headers.insert("Prefer", "handling=strict");

		QString url = "https://" + QString(test_server ? "tc-t" : "tc-p") + ".med.uni-tuebingen.de/auth/realms/trustcenter/protocol/openid-connect/token";
		if (debug) addOutputLine("URL: "+url);
		QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("pseudo_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("pseudo_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
		if (debug) addOutputLine("data: " + data);

		HttpHandler handler(true);
		QByteArray reply = handler.post(url, data, headers);
		QByteArrayList parts = reply.split('"');
		if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

		token = parts[3];
		if (debug) addOutputLine("Token: " + token);
	}

	//get first pseudonyms
	if (debug) addOutputLine("");
	if (debug) addOutputLine("String to encode: " + str);
	QByteArray pseudo1 = "";
	if (skip_pseudo1)
	{
		pseudo1 = str;
	}
	else
	{
		QString url = "https://tc-" + QString(test_server ? "t" : "p") + ".med.uni-tuebingen.de/v1/process?targetSystem=MVH_"+(test_server ? "T" : "P")+"_F";
		if (debug) addOutputLine("URL: "+url);

		HttpHeaders headers;
		headers.insert("Content-Type", "application/json");
		headers.insert("Authorization", "Bearer "+token);

		HttpHandler handler(true);
		QByteArray data =  jsonDataPseudo(str);
		if (debug) addOutputLine("data: " + data);
		QByteArray reply = handler.post(url, data, headers);
		if (debug) addOutputLine("reply: " + reply);
		pseudo1 = parseJsonDataPseudo(reply, "first_level");
	}
	if (debug) addOutputLine("Pseudonym 1: " + pseudo1);

	//get second pseudonyms
	QByteArray pseudo2 = "";
	{
		QString url = "https://tc-" + QString(test_server ? "t" : "p") + ".med.uni-tuebingen.de/v1/process?targetSystem=MVH_"+(test_server ? "T" : "P")+"_"+(context=="GRZ" ? "G" : "SE");
		if (debug) addOutputLine("URL: "+url);

		HttpHeaders headers;
		headers.insert("Content-Type", "application/json");
		headers.insert("Authorization", "Bearer "+token);

		HttpHandler handler(true);
		QByteArray data =  jsonDataPseudo(pseudo1);
		if (debug) addOutputLine("data: " + data);
		QByteArray reply = handler.post(url, data, headers);
		pseudo2 = parseJsonDataPseudo(reply, "second_level");
	}
	if (debug) addOutputLine("Pseudonym 2: " + pseudo2);

	return pseudo2;
}

void MVHub::loadDataFromCM(int debug_level)
{
	try
	{
		addOutputHeader("loading case-management data from RedCap", false);

		QHash<QString, QStringList> cmid2data;
		QHash<QString, QString> cmid2sapid;
		int c_cm_consent = colOf("consent signed [CM]");
		int c_confirmation = colOf("export confirmation");

		//get RedCap data from API
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		QByteArray data = "token="+Settings::string("redcap_casemanagement_token").toLatin1()+"&content=record&rawOrLabel=label";
		HttpHandler handler(true);
		QByteArrayList reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers).split('\n');

		//get IDs of entries currently in RedCap
		QSet<QString> cm_ids_redcap;
        foreach(QByteArray line, reply)
		{
			if (!line.startsWith("<item>")) continue;

			//open file
            QDomDocument doc = XmlHelper::parse(line);
			QDomElement root = doc.documentElement();
			QString cm_id = root.namedItem("record_id").toElement().text().trimmed();
			cm_ids_redcap << cm_id;
		}

		//delete database entries that are no longer in RedCap
		NGSD mvh_db(true, "mvh");
		QStringList cm_ids_db = mvh_db.getValues("SELECT cm_id FROM case_data WHERE id NOT IN (SELECT case_id FROM submission_grz) AND id NOT IN (SELECT case_id FROM submission_kdk_se)");
		foreach(QString cm_id, cm_ids_db)
		{
			if (cm_ids_redcap.contains(cm_id)) continue;

			addOutputLine("Deleting sample with CM ID '" + cm_id + "': no longer contained in CM RedCap!");
			mvh_db.getQuery().exec("DELETE FROM case_data WHERE cm_id='" + cm_id + "'");
		}

		//add/update database entries with up-to-date data from RedCap API
        foreach(QByteArray line, reply)
		{
			if (!line.startsWith("<item>")) continue;

			//open file
            QDomDocument doc = XmlHelper::parse(line);
			QDomElement root = doc.documentElement();
			QString cm_id = root.namedItem("record_id").toElement().text().trimmed();

			//skip items flagged as 'redcap_repeat_instance'
			if(root.namedItem("redcap_repeat_instance").toElement().text().trimmed()!="")
			{
				if (debug_level>=2) addOutputLine("Skipped repeat instance for " + cm_id);
				continue;
			}

			//cache SAP ID
			cmid2data[cm_id] << line;
			QString sap_id = root.namedItem("pat_id").toElement().text().trimmed();
			if (!sap_id.isEmpty()) cmid2sapid[cm_id] = sap_id;

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
				if (tag=="record_id")
				{
					QString cm_id = e.text().trimmed();
					QTableWidgetItem* item = GUIHelper::createTableItem(cm_id);
					QString id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='" + cm_id + "'").toString();
					item->setToolTip("id: "+id);
					ui_.table->setItem(r, 0, item);
				}
				if (tag=="case_id") ui_.table->setItem(r, 1, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="status_study") ui_.table->setItem(r, 2, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="pat_id") ui_.table->setItem(r, 3, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="network_title") ui_.table->setItem(r, 4, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="seq_mode") ui_.table->setItem(r, 6, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="sample_arrival_date") ui_.table->setItem(r, 7, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="seq_state") ui_.table->setItem(r, 8, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="gen_finding_date") ui_.table->setItem(r, 9, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="datum_kuendigung_te") ui_.table->setItem(r, 10, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="bc_signed") ui_.table->setItem(r, c_cm_consent, GUIHelper::createTableItem(e.text().trimmed()));
				if (tag=="status_complete" && e.text().trimmed()!="Complete")
				{
					QTableWidgetItem* item = ui_.table->item(r,2);
					if (item!=nullptr) item->setToolTip("CM docu incomplete");
					else qDebug() << cm_id << "item with index 2 missing";
				}

				n = n.nextSibling();
			}
		}

		//insert/update data in MVH database
		SqlQuery query = mvh_db.getQuery();
		query.prepare("INSERT INTO case_data (cm_id, cm_data, sap_id) VALUES (:0,:1,:2) ON DUPLICATE KEY UPDATE cm_data=VALUES(cm_data), sap_id=VALUES(sap_id)");
		foreach(QString cm_id, cmid2sapid.keys())
		{
			QString sap_id = cmid2sapid[cm_id];

			//check that SAP ID is not used by other sample
			QString mvh_case_id = mvh_db.getValue("SELECT cm_id FROM case_data WHERE sap_id='"+sap_id+"'").toString();
			if (!mvh_case_id.isEmpty() && mvh_case_id!=cm_id)
			{
				addOutputLine("Skipping sample with CM ID '" + cm_id + "': SAP ID '"+sap_id+"' already used by sample with CM ID '" + mvh_case_id + "'!");
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

		//update export confirmation (from repeat elements)
		QHash<QString, QStringList> cmid2conf;
        foreach(QByteArray line, reply)
		{
			if (!line.startsWith("<item>")) continue;

			//open file
            QDomDocument doc = XmlHelper::parse(line);
			QDomElement root = doc.documentElement();
			QString cm_id = root.namedItem("record_id").toElement().text().trimmed();

			//get confirmation data
			QString type = root.namedItem("report_data_type").toElement().text();
			if (type.isEmpty()) continue;
			if (type=="klinisch") type = "KDK";
			if (type=="genomisch") type = "GRZ";

			QString date = root.namedItem("report_date").toElement().text();
			if (date.isEmpty()) continue;

			QString result = root.namedItem("report_dq").toElement().text();
			if (result!="Bestanden") continue;

			cmid2conf[cm_id] << (type + " " +date);
		}
		for(auto it = cmid2conf.begin(); it!=cmid2conf.end(); ++it)
		{
			QString cm_id = it.key();

			ui_.table->setItem(rowOf(cm_id), c_confirmation, GUIHelper::createTableItem(it.value().join(" // ")));
		}
	}
	catch (Exception& e)
	{
		addOutputLine("ERROR:");
		addOutputLine(e.message());
	}
}

void MVHub::loadDataFromSE()
{
	try
	{
		addOutputHeader("loading SE data from RedCap", false);

		QTextStream stream(stdout);
		QHash<QString, QString> psn2sap;
		QHash<QString, QString> psn2consent_version;
		QHash<QString, QStringList> psn2items;

		//get data from RedCap API
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		QByteArray data = "token="+Settings::string("redcap_se_token").toLatin1()+"&content=record&rawOrLabel=label";
		HttpHandler handler(true);
        QByteArray reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers);

		//parse items
        QDomDocument doc = XmlHelper::parse(reply);
		QDomElement root = doc.documentElement();
		QDomNode n = root.firstChild();
		while(!n.isNull())
		{
			QDomElement e = n.toElement(); // try to convert the node to an element.
			if(e.isNull()) continue;

			QString psn_id = e.namedItem("psn").toElement().text().trimmed();
			QString sap_id = e.namedItem("pat_id").toElement().text().trimmed(); //attention: not contained in repeat elements!

			//store network id to SAP ID mapping (if not repeat element)
			if (!sap_id.isEmpty()) psn2sap[psn_id] = sap_id;

			//store consent version in datastructure (in repeat element)
			QString consent_version = e.namedItem("vers_einwillig_forsch").toElement().text().trimmed();
			if (!consent_version.isEmpty())
			{
				psn2consent_version[psn_id] = consent_version;
			}

			//store SE data in datastructure
			QString item;
			QTextStream stream(&item);
			e.save(stream, 0);
			stream.flush();
			psn2items[psn_id] << item;

			n = n.nextSibling();
		}

		//create mapping SAP > PSN
		QHash<QString, QStringList> sap2psn;
		for(auto it=psn2sap.begin(); it!=psn2sap.end(); ++it)
		{
			QString psn_id = it.key();
			QString sap_id = it.value();
			if (sap_id.isEmpty()) continue;

			sap2psn[sap_id] << psn_id;
		}
		//check for duplicate SAP IDs
		for(auto it=sap2psn.begin(); it!=sap2psn.end(); ++it)
		{
			if (it.value().count()>1)
			{
				QString sap_id = it.key();
				addOutputLine("Skipping samples with SAP ID '" + sap_id +"' - SAP ID used several times: "+it.value().join(", "));
				sap2psn[sap_id].clear();
			}
		}

		//store data in MVH database
		for(auto it=psn2sap.begin(); it!=psn2sap.end(); ++it)
		{
			QString psn_id = it.key();
			QString sap_id = it.value();
			if (sap_id.isEmpty())
			{
				addOutputLine("Skipping sample with SE ID '" + psn_id + "': no SAP ID available!");
				continue;
			}

			NGSD mvh_db(true, "mvh");
			QString mvh_id = mvh_db.getValue("SELECT id FROM case_data WHERE sap_id='"+sap_id+"'").toString();
			if (mvh_id.isEmpty())
			{
				addOutputLine("Skipping sample with SE ID '" + psn_id + "': no entry in MVH case_data table for SAP id '"+sap_id+"'!");
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
		int c_sap_id = colOf("SAP ID");
		int c_network_id = colOf("Netzwerk ID");
		int c_consent_ver = colOf("consent version [SE]");
		for(int r=0; r<ui_.table->rowCount(); ++r)
		{
			QString sap_id = getString(r, c_sap_id);
			if (sap_id=="") continue;

			if (sap2psn[sap_id].count()==1)
			{
				QString network_id = sap2psn[sap_id].first();
				ui_.table->setItem(r, c_network_id, GUIHelper::createTableItem(network_id));
				ui_.table->setItem(r, c_consent_ver, GUIHelper::createTableItem(psn2consent_version[network_id]));
			}
		}
	}
	catch (Exception& e)
	{
		addOutputLine("ERROR:");
		addOutputLine(e.message());
	}
}

QList<MVHub::VarData> MVHub::getVariants(NGSD& db, QString ps)
{
	QList<VarData> output;

	QString processed_sample_id = db.processedSampleId(ps);

	int rc_id = db.reportConfigId(processed_sample_id);
	if (rc_id!=-1)
	{
		//find causal small variants
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_variant WHERE causal='1' AND report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				QString var_id = query.value("variant_id").toString();
				Variant var = db.variant(var_id);
				GeneSet genes = variantGenes(db, var.chr(), var.start(), var.end());

				VarData var_data;
				var_data.name = (var.toString() + " (" + genes.join(", ") + ")").toUtf8();
				var_data.localization = variantLocalization(db, var.chr(), var.start(), var.end(), genes);
				var_data.type = VarType::CAUSAL;

				output << var_data;
			}
		}

		//find causal CNVs
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_cnv WHERE causal='1' AND report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				QString cnv_id = query.value("cnv_id").toString();
				CopyNumberVariant var = db.cnv(cnv_id.toInt());
				QString cn = db.getValue("SELECT cn FROM cnv WHERE id='" + cnv_id + "'").toString();
				GeneSet genes = variantGenes(db, var.chr(), var.start(), var.end());

				VarData var_data;
				QString genes_str = genes.count()<10 ? genes.join(", ") : "many";
				var_data.name = ("CNV: " + var.toString() + " CN=" + cn + " size=" + QString::number((var.end()-var.start())/1000.0, 'f', 3) + "kb (" + genes_str + ")").toUtf8();
				var_data.localization = variantLocalization(db, var.chr(), var.start(), var.end(), genes);
				var_data.type = VarType::CAUSAL;

				output << var_data;
			}
		}

		//find causal SVs
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_sv WHERE causal='1' AND report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				//determine type and ID in type-specific table of SV
				StructuralVariantType type = StructuralVariantType::UNKNOWN;
				int sv_id = -1;
				if (query.value("sv_deletion_id").toInt()!=0)
				{
					type = StructuralVariantType::DEL;
					sv_id = query.value("sv_deletion_id").toInt();
				}
				if (query.value("sv_duplication_id").toInt()!=0)
				{
					type = StructuralVariantType::DUP;
					sv_id = query.value("sv_duplication_id").toInt();
				}
				if (query.value("sv_insertion_id").toInt()!=0)
				{
					type = StructuralVariantType::INS;
					sv_id = query.value("sv_insertion_id").toInt();
				}
				if (query.value("sv_inversion_id").toInt()!=0)
				{
					type = StructuralVariantType::INV;
					sv_id = query.value("sv_inversion_id").toInt();
				}
				//missing: BNDs
				if (query.value("sv_translocation_id").toInt()!=0)
				{
					continue;
				}

				//get variant and genotype
				BedpeFile svs;
				svs.setAnnotationHeaders(QList<QByteArray>() << "FORMAT" << ps.toUtf8()); //FROMAT column that will contain the genotype
				BedpeLine var = db.structuralVariant(sv_id, type, svs, true);
				QString genotype = var.annotations()[1];
				if (genotype=="1/1") genotype = "hom";
				if (genotype=="0/1") genotype = "het";
				BedLine reg = var.affectedRegion()[0];
				GeneSet genes = variantGenes(db, reg.chr(), reg.start(), reg.end());

				VarData var_data;
				QString genes_str = genes.count()<10 ? genes.join(", ") : "many";
				var_data.name = ("SV-" + var.toString(true) + " genotype=" + genotype + " (" + genes_str + ")").toUtf8();
				var_data.localization = variantLocalization(db, reg.chr(), reg.start(), reg.end(), genes);
				var_data.type = VarType::CAUSAL;

				output << var_data;
			}
		}

		//find causal REs
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT re.name, re.region, reg.allele1, reg.allele2 FROM report_configuration_re rcr, repeat_expansion_genotype reg, repeat_expansion re WHERE re.id=reg.repeat_expansion_id AND reg.id=rcr.repeat_expansion_genotype_id AND rcr.causal='1' AND rcr.report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				QByteArray a1 = query.value("allele1").toByteArray();
				QByteArray a2 = query.value("allele2").toByteArray();
				QByteArray name = "RE: " + query.value("name").toByteArray() + " length=" + a1;
				if (!a2.isEmpty()) name += "/" + a2;
				BedLine reg = BedLine::fromString(query.value("region").toByteArray());
				GeneSet genes = variantGenes(db, reg.chr(), reg.start(), reg.end());

				VarData var_data;
				QByteArray genes_str = genes.count()<10 ? genes.join(", ") : "many";
				var_data.name = name + " (" + genes_str + ")";
				var_data.localization = variantLocalization(db, reg.chr(), reg.start(), reg.end(), genes);
				var_data.type = VarType::CAUSAL;

				output << var_data;
			}
		}

		//missing: add other causal variants
		//missing: VUS/INCIDENTAL variants (they are optional - see email Corinna Ernst from 2025-08-11)
	}
	return output;
}

GeneSet MVHub::variantGenes(NGSD& db, const Chromosome& chr, int start, int end)
{
	GeneSet genes = db.genesOverlapping(chr, start, end, 0);
	if (genes.isEmpty()) genes = db.genesOverlapping(chr, start, end, 5000);

	return genes;
}

QByteArray MVHub::variantLocalization(NGSD& db, const Chromosome& chr, int start, int end, const GeneSet& genes)
{
	//runtime optimization for largs CNVs/SVs
	if (genes.count()>=10) return "coding-region";

	//exon region
	BedFile exon_regions = db.genesToRegions(genes, Transcript::ENSEMBL, "exon");
	exon_regions.merge();
	if (exon_regions.overlapsWith(chr, start, end)) return "coding-region";

	//splice reion
	BedFile splicing_regions = exon_regions;
	splicing_regions.extend(20);
	splicing_regions.merge();
	splicing_regions.subtract(exon_regions);
	if (splicing_regions.overlapsWith(chr, start, end)) return "splicing-region";

	//intronic region
	BedFile gene_loci = db.genesToRegions(genes, Transcript::ENSEMBL, "gene");
	if (gene_loci.overlapsWith(chr, start, end)) return "intronic";

	//missing: "regulatory-region" because it is not tracked in NGSD

	return "intergenic"; //fallback
}

MVHub::Network MVHub::getNetwork(int r)
{
	static int c = -1;
	if (c==-1) c= colOf("Netzwerk");

	QString network = getString(r, c);
	if (network=="Netzwerk Seltene Erkrankungen") return SE;
	if (network=="Deutsches Netzwerk für Personalisierte Medizin") return OE;
	if (network=="Deutsches Konsortium Familiärer Brust- und Eierstockkrebs") return FBREK;
	if (network=="") return UNSET;
	THROW(ArgumentException, "Unhandled network type '" + network +"'!");
}

QString MVHub::networkToString(Network network)
{
	switch(network)
	{
		case SE:
			return "Netzwerk Seltene Erkrankungen";
		case OE:
			return "Deutsches Netzwerk für Personalisierte Medizin";
		case FBREK:
			return "Deutsches Konsortium Familiärer Brust- und Eierstockkrebs";
		case UNSET:
			return "";
	}
	THROW(ProgrammingException, "Unhandled network enum " + QString::number(network));
}

int MVHub::rowOf(QString cm_id)
{
	static int c = -1;
	if (c==-1) c= colOf("CM ID");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		if (getString(r, c)==cm_id) return r;
	}

	return -1;
}

int MVHub::colOf(QString col, bool throw_if_not_found)
{
	return GUIHelper::columnIndex(ui_.table, col, throw_if_not_found);
}
