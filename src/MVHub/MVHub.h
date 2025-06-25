#ifndef MVHUB_H
#define MVHUB_H

#include <QMainWindow>
#include "DelayedInitializationTimer.h"
#include "ui_MVHub.h"

///Main window class
class MVHub
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MVHub(QWidget* parent = 0);

public slots:
	void delayedInitialization();
	void tableContextMenu(QPoint pos);
	void updateTableFilters();
	void test_apiPseudo();
	//load research consent data from meDIC
	void loadConsentData();
	//load data from GenLab
	void loadGenLabData();
	//export consent data for a sample list
	void exportConsentData();
	//check XML data in MVH database
	void checkXML();

private:
	Ui::MVHub ui_;
	DelayedInitializationTimer delayed_init_;
	QHash<QString, QStringList> cmid2messages_;

	//clear output panel on bottom
	void addOutputHeader(QString section, bool clear=true);

	//load data Modellvorhaben case management RedCap and copy it to MVH database
	void loadDataFromCM();
	//load data Modellvorhaben SE RedCap and copy it to MVH database
	void loadDataFromSE();
	//determine processed samples for cases from NGSD
	void determineProcessedSamples();
	//show messages
	void showMessages();

	//returns consent status of patient. Empty string if not available.
	QString getConsent(QString sap_id, bool return_parsed_data = true, bool debug=false);
	//parse JSON and convert it to XML
	QByteArray parseConsentJson(QByteArray json_text);


	//creates JSON input for pseudonymization
	static QByteArray jsonDataPseudo(QByteArray str);
	//parses JSON output of pseudonymization
	QByteArray parseJsonDataPseudo(QByteArray reply, QByteArray context);

	//returns the string contents of a table item
	QString getString(int r, int c, bool trim=true);
};

#endif // MVHUB_H
