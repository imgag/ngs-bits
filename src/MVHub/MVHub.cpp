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

	connect(ui_.api_consent, SIGNAL(clicked()), this, SLOT(test_apiConsent()));
	connect(ui_.api_pseudo, SIGNAL(clicked()), this, SLOT(test_apiPseudo()));
	connect(ui_.api_redcap_case, SIGNAL(clicked()), this, SLOT(test_apiReCapCaseManagement()));
}

void MVHub::delayedInitialization()
{
	loadSamplesFromNGSD();
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

QByteArray MVHub::parseJsonDataPseudo(QByteArray reply)
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

		QByteArray encrypted = QByteArray::fromBase64(object["identifier"].toArray()[0].toObject()["value"].toString().toLatin1());


		/*
		with open(private_key_file, "rb") as f:
			   rsa_private_key = RSA.importKey(f.read())
			   rsa_private_key = PKCS1_v1_5.new(rsa_private_key)
			   sentinel = get_random_bytes(16)
			   return rsa_private_key.decrypt(base64.b64decode(enc_msg), sentinel).decode('utf-8')
		*/


		return encrypted;
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

void MVHub::test_apiConsent()
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
		ui_.output->appendPlainText("Getting token...");
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		headers.insert("Prefer", "handling=strict");
		QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("consent_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("consent_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
		QString url = test_server ? "https://tc-t.med.uni-tuebingen.de/auth/realms/consent-test/protocol/openid-connect/token" : "https://tc-p.med.uni-tuebingen.de/auth/realms/consent/protocol/openid-connect/token";
		ui_.output->appendPlainText("URL: "+url);
		ui_.output->appendPlainText("data: " + data);
		HttpHandler handler(true);
		QByteArray reply = handler.post(url, data, headers);
		QByteArrayList parts = reply.split('"');
		if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

		QByteArray token = parts[3];
		ui_.output->appendPlainText("Token: "+ token);

		//get consent
		ui_.output->appendPlainText("Getting consent...");
		QString ps = "DNA2411657A1_01";
		GenLabDB genlab;
		QString sap_id = genlab.sapID(ps);
		HttpHeaders headers2;
		headers2.insert("Authorization", "Bearer "+token);
		QString url2 = test_server ? "https://tc-t.med.uni-tuebingen.de:8443/fhir/Consent" : "https://tc-p.med.uni-tuebingen.de:8443/fhir/Consent";
		reply = handler.get(url2+"?patient.identifier="+sap_id, headers2);

		ui_.output->appendPlainText("IMGAG ID: " + ps);
		ui_.output->appendPlainText("SAP ID:" + sap_id);
		ui_.output->appendPlainText("Consent:");
		ui_.output->appendPlainText(reply);
	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR:");
		ui_.output->appendPlainText(e.message());
	}
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
			QString url = test_server ? "https://tc-t.med.uni-tuebingen.de/auth/realms/trustcenter/protocol/openid-connect/token" : "https://tc-p.med.uni-tuebingen.de/auth/realms/trustcenter/protocol/openid-connect/token";
			ui_.output->appendPlainText("URL: "+url);
			ui_.output->appendPlainText("data: " + data);

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
			QString url = "https://tc.medic-tuebingen.de/v1/process?targetSystem=MVH_T_F";

			HttpHeaders headers;
			headers.insert("Content-Type", "application/json");
			headers.insert("Authorization", "Bearer "+token);

			HttpHandler handler(false);
			QByteArray data =  jsonDataPseudo(str);
			QByteArray reply = handler.post(url, data, headers);
			pseudo1 = parseJsonDataPseudo(reply);
		}
		ui_.output->appendPlainText("Pseudonym 1: " + pseudo1);

		//get second pseudonyms
		//TODO

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
