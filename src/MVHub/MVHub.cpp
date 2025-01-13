#include "MVHub.h"
#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "XmlHelper.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "GenLabDB.h"

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
		bool test_server = false;
		if (test_server) ui_.output->appendPlainText("NOTE: using test server!");

		//get token
		ui_.output->appendPlainText("Getting token...");
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		headers.insert("Prefer", "handling=strict");
		QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("consent_client_id"+QString(test_server ? "_test" : "")).toLatin1()+"&client_secret="+Settings::string("consent_client_secret"+QString(test_server ? "_test" : "")).toLatin1();
		HttpHandler handler(true);
		QString url = test_server ? "https://tc-t.med.uni-tuebingen.de/auth/realms/consent-test/protocol/openid-connect/token" : "https://tc-p.med.uni-tuebingen.de/auth/realms/consent/protocol/openid-connect/token";
		QByteArray reply = handler.post(url, data, headers);
		QByteArrayList parts = reply.split('"');
		if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

		QByteArray token = parts[3];
		ui_.output->appendPlainText("Token:");
		ui_.output->appendPlainText(token);

		//get consent
		ui_.output->appendPlainText("Getting consent...");
		QString ps = "DX2103725_02";
		QString sap_id = "6068866";
		HttpHeaders headers2;
		headers2.insert("Authorization", "Bearer "+token);
		QString url2 = test_server ? "https://tc-t.med.uni-tuebingen.de:8443/fhir/Consent" : "https://tc-p.med.uni-tuebingen.de:8443/fhir/Consent";
		reply = handler.get(url2+"?patient.identifier="+sap_id, headers2);

		ui_.output->appendPlainText("ID: " + ps);
		ui_.output->appendPlainText("SAP:" + sap_id);
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

		//get token
		ui_.output->appendPlainText("Getting token...");
		HttpHeaders headers;
		headers.insert("Content-Type", "application/x-www-form-urlencoded");
		headers.insert("Prefer", "handling=strict");
		QByteArray data = "grant_type=client_credentials&client_id="+Settings::string("pseudo_client_id").toLatin1()+"&client_secret="+Settings::string("pseudo_client_secret").toLatin1();
		HttpHandler handler(true);
		QByteArray reply = handler.post("https://tc-t.med.uni-tuebingen.de/auth/realms/trustcenter/protocol/openid-connect/token", data, headers);
		QByteArrayList parts = reply.split('"');
		if (parts.count()<4) THROW(Exception, "Reply could not be split by '\"' into 4 parts:\n" + reply);

		QByteArray token = parts[3];
		ui_.output->appendPlainText("Token:");
		ui_.output->appendPlainText(token);

		//get pseudonyms
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
