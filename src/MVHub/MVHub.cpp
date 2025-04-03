#include "MVHub.h"
#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "XmlHelper.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "GenLabDB.h"

#include <QJsonArray>
#include <QJsonDocument>

MVHub::MVHub(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, delayed_init_(this, true)
{
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());

	connect(ui_.update_consent, SIGNAL(clicked()), this, SLOT(updateConsentData()));
	connect(ui_.api_pseudo, SIGNAL(clicked()), this, SLOT(test_apiPseudo()));
	connect(ui_.api_redcap_case, SIGNAL(clicked()), this, SLOT(test_apiReCapCaseManagement()));
	connect(ui_.table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableContextMenu(QPoint)));
}

void MVHub::delayedInitialization()
{
	loadSamplesFromNGSD();
}

void MVHub::tableContextMenu(QPoint pos)
{
	//execute menu
	QMenu menu;
	QAction* a_copy = menu.addAction("Copy all");
	QAction* action = menu.exec(ui_.table->viewport()->mapToGlobal(pos));
	if (action==nullptr) return;

	if  (action==a_copy)
	{
		GUIHelper::copyToClipboard(ui_.table);
	}
}

void MVHub::updateConsentData()
{
	clearOutput(sender());

	int c_consent = GUIHelper::columnIndex(ui_.table, "Consent");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		QString ps = ui_.table->item(r,0)->text();
		QString consent = getConsent(ps, false);

		QTableWidgetItem* item = GUIHelper::createTableItem(consent);
		ui_.table->setItem(r, c_consent, item);
	}

	ui_.table->resizeColumnToContents(c_consent);
	ui_.table->resizeRowsToContents();
}

void MVHub::loadSamplesFromNGSD()
{
	NGSD db;
	GenLabDB genlab;
	ProcessedSampleSearchParameters params;
	params.s_study = "Modellvorhaben_2024";
	params.include_archived_projects = false;
	params.run_finished = true;
	params.include_bad_quality_samples = false;
	DBTable res = db.processedSampleSearch(params);
	int i_ps = res.columnIndex("name");
	int i_sys_type = res.columnIndex("system_type");
	int i_project_name = res.columnIndex("project_name");
	int i_is_tumor = res.columnIndex("is_tumor");
	int i_disease_status = res.columnIndex("disease_status");
	for(int i=0; i<res.rowCount(); ++i)
	{
		QString ps_id = res.row(i).id();
		QString ps = res.row(i).value(i_ps);
		QString project = res.row(i).value(i_project_name);
		bool is_somatic = project=="SomaticAndTreatment";
		QString is_tumor = res.row(i).value(i_is_tumor);
		QString sys_type = res.row(i).value(i_sys_type);
		QString disease_status = res.row(i).value(i_disease_status);
		if (is_somatic && (sys_type=="RNA" || is_tumor=="no")) continue; //for onko: only tumor DNA samples
		if (!is_somatic && disease_status!="Affected") continue; //for gemline: only affected

		int r = ui_.table->rowCount();
		ui_.table->setRowCount(r+1);

		//PS name
		ui_.table->setItem(r, 0, GUIHelper::createTableItem(ps));

		//type and additional samples
		QString type;
		QStringList add_samples;
		if (is_somatic)
		{
			type = "T/N";

			add_samples << "normal:"+db.normalSample(ps_id);
			QString rna = db.rna(ps_id, false);
			if (rna!="") add_samples << "rna:"+rna;
		}
		else
		{
			type = "germline";

			QString f = db.father(ps_id, false);
			if (f!="") add_samples << "father:"+f;
			QString m = db.mother(ps_id, false);
			if (m!="") add_samples << "mother:"+m;
		}
		ui_.table->setItem(r, 1, GUIHelper::createTableItem(type));
		ui_.table->setItem(r, 2, GUIHelper::createTableItem(add_samples.join(", ")));

		//SAP IDs
		QString sap_id = genlab.sapID(ps);
		ui_.table->setItem(r, 3, GUIHelper::createTableItem(sap_id));

		//Case management information
		//TODO
	}

	GUIHelper::resizeTableCellWidths(ui_.table, 400, 20);
	GUIHelper::resizeTableCellHeightsToMinimum(ui_.table, 20);

}

QString MVHub::getSAP(QString ps, bool padded)
{
	int c_sap = GUIHelper::columnIndex(ui_.table, "SAP ID");

	for (int r=0; r<ui_.table->rowCount(); ++r)
	{
		if (ui_.table->item(r,0)->text()==ps)
		{
			QString id = ui_.table->item(r,c_sap)->text();
			while (padded && id.size()<10) id.prepend('0');
			return id;
		}
	}

	return "";
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
		//ui_.output->appendPlainText("base64-encoded reply: " + str_base64);

		/*
			QFile file2("T:\\users\\ahsturm1\\scripts\\2024_05_06_MVH\\encrypted_base64.txt");
			file2.open(QIODevice::WriteOnly);
			file2.write(str_base64);
			file2.close();
		*/

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

void MVHub::clearOutput(QObject* sender)
{
	ui_.output->clear();

	QPushButton* button = qobject_cast<QPushButton*>(sender);
	if (button!=nullptr)
	{
		ui_.output->appendPlainText("### " + button->text() + " ###");
		ui_.output->appendPlainText("");
	}
}

QString MVHub::getConsent(QString ps, bool debug)
{
	try
	{
		//test or production
		bool test_server = false;
		if (test_server)
		{
			ui_.output->appendPlainText("ATTENTION: using test servers for getConsent");
			ui_.output->appendPlainText("");
		}

		//get token if missing
		static QByteArray token = "";
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
		ui_.output->appendPlainText("Getting consent for "+ps+"...");
		QString sap_id = getSAP(ps, true);
		HttpHeaders headers2;
		headers2.insert("Authorization", "Bearer "+token);
		QString url = test_server ? "https://tc-t.med.uni-tuebingen.de:8443/fhir/Consent" : "https://tc-p.med.uni-tuebingen.de:8443/fhir/Consent";
		url += "?patient.identifier="+sap_id;
		HttpHandler handler(true);
		QByteArray reply = handler.get(url, headers2);
		if (debug)
		{
			ui_.output->appendPlainText("URL:" + url);
			ui_.output->appendPlainText("SAP ID:" + sap_id);
			ui_.output->appendPlainText("Consent:" + reply);
		}

		return parseConsentJson(reply);
	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());
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

		QByteArray status = object["status"].toString().toLatin1();
		QByteArray date = object["dateTime"].toString().left(10).toLatin1();
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
					allowed << v2.toObject()["display"].toString().toLatin1();
				}
			}
		}

		output << (date+"/"+status+": "+allowed.join(", "));
	}
	return output.join("\n");
}

void MVHub::test_apiPseudo()
{
	try
	{
		clearOutput(sender());

		//test or production
		bool test_server = true;
		if (test_server)
		{
			ui_.output->appendPlainText("ATTENTION: using test server!");
			ui_.output->appendPlainText("");
		}

		//get token
		QByteArray token = "";
		{
			ui_.output->appendPlainText("Getting token...");

			HttpHeaders headers;
			headers.insert("Content-Type", "application/x-www-form-urlencoded");
			headers.insert("Prefer", "handling=strict");

			QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("pseudo_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("pseudo_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
			QString url = "https://" + QString(test_server ? "tc-t" : "tc-p") + ".med.uni-tuebingen.de/auth/realms/trustcenter/protocol/openid-connect/token";
			ui_.output->appendPlainText("URL: "+url);
			//ui_.output->appendPlainText("data: " + data);

			HttpHandler handler(true);
			QByteArray reply = handler.post(url, data, headers);
			QByteArrayList parts = reply.split('"');
			if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

			token = parts[3];
			ui_.output->appendPlainText("Token: " + token);
		}

		//get first pseudonyms
		QByteArray str = "ABCDE";
		ui_.output->appendPlainText("");
		ui_.output->appendPlainText("String to encode: " + str);
		QByteArray pseudo1 = "";
		{
			QString url = "https://" + QString(test_server ? "tc-t.med.uni-tuebingen.de" : "tc.medic-tuebingen.de") + "/v1/process?targetSystem=MVH_T_F";
			ui_.output->appendPlainText("URL: "+url);

			HttpHeaders headers;
			headers.insert("Content-Type", "application/json");
			headers.insert("Authorization", "Bearer "+token);

			HttpHandler handler(test_server);
			QByteArray data =  jsonDataPseudo(str);
			QByteArray reply = handler.post(url, data, headers);
			pseudo1 = parseJsonDataPseudo(reply, "T_F");
		}
		ui_.output->appendPlainText("Pseudonym 1: " + pseudo1);

		//get second pseudonyms
		QByteArray pseudo2 = "";
		{
			QString url = "https://" + QString(test_server ? "tc-t.med.uni-tuebingen.de" : "tc.medic-tuebingen.de") + "/v1/process?targetSystem=MVH_T_SE";
			ui_.output->appendPlainText("URL: "+url);

			HttpHeaders headers;
			headers.insert("Content-Type", "application/json");
			headers.insert("Authorization", "Bearer "+token);

			HttpHandler handler(test_server);
			QByteArray data =  jsonDataPseudo(pseudo1);
			QByteArray reply = handler.post(url, data, headers);
			pseudo2 = parseJsonDataPseudo(reply, "T_SE");
		}
		ui_.output->appendPlainText("Pseudonym 2: " + pseudo2);

	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());
	}
}



void MVHub::test_apiReCapCaseManagement()
{
	try
	{
		clearOutput(sender());

		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		QByteArray data = "token="+Settings::string("redcap_casemanagement_token").toLatin1()+"&content=record";
		HttpHandler handler(true);
		QByteArray reply = handler.post("https://redcap.extern.medizin.uni-tuebingen.de/api/", data, headers);

		ui_.output->appendPlainText("reply:");
		ui_.output->appendPlainText(XmlHelper::format(reply));

	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());
	}
}
