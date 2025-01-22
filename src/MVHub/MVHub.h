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

	void updateConsentData();
	void test_apiPseudo();
	void test_apiReCapCaseManagement();

private:
	Ui::MVHub ui_;
	DelayedInitializationTimer delayed_init_;

	//clear output panel on bottom
	void clearOutput(QObject* sender);
	//load samples in Modellvorhaben from NGSD
	void loadSamplesFromNGSD();
	//annotate consent data of sample

	//get SAP patient ID for processed sample. Empty string if not available.
	QString getSAP(QString ps, bool padded);

	//get consent status of patient. Empty string if not available.
	QString getConsent(QString ps, bool debug);
	static QByteArray parseConsentJson(QByteArray json_text);


	//creates JSON input for pseudonymization
	static QByteArray jsonDataPseudo(QByteArray str);
	//parses JSON output of pseudonymization
	QByteArray parseJsonDataPseudo(QByteArray reply, QByteArray context);
};

#endif // MVHUB_H
