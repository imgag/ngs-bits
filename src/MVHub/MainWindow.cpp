#include "MainWindow.h"
#include "HttpHandler.h"
#include "Exceptions.h"
#include "Settings.h"
#include "XmlHelper.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
{
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());

	connect(ui_.api_consent, SIGNAL(clicked()), this, SLOT(test_apiConsent()));
	connect(ui_.api_pseudo, SIGNAL(clicked()), this, SLOT(test_apiPseudo()));
	connect(ui_.api_redcap_case, SIGNAL(clicked()), this, SLOT(test_apiReCapCaseManagement()));
}

void MainWindow::clearOutput(QObject* sender)
{
	ui_.output->clear();

	QPushButton* button = qobject_cast<QPushButton*>(sender);
	if (button!=nullptr)
	{
		ui_.output->appendPlainText("### " + button->text() + " ###");
		ui_.output->appendPlainText("");
	}
}

void MainWindow::test_apiConsent()
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

void MainWindow::test_apiPseudo()
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

void MainWindow::test_apiReCapCaseManagement()
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
