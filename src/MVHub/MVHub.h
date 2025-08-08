#ifndef MVHUB_H
#define MVHUB_H

#include <QMainWindow>
#include "DelayedInitializationTimer.h"
#include "ui_MVHub.h"
#include "NGSD.h"

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
	//pseudonymization function
	QByteArray getPseudonym(QByteArray str, QByteArray context, bool test_server = true, bool debug = true);
	//load research consent data from meDIC
	void loadConsentData();
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
	//add output line and make sure the new line is visible to the user
	void addOutputLine(QString line);

	//load data Modellvorhaben case management RedCap and copy it to MVH database
	void loadDataFromCM();
	//load data Modellvorhaben SE RedCap and copy it to MVH database
	void loadDataFromSE();
	//determine processed samples for cases from NGSD
	void determineProcessedSamples();
	//add missing HPO terms to SE RedCap and update SE data in MVH database
	int updateHpoTerms(bool debug=false);
	//add missing variants to SE RedCap and update SE data in MVH database
	int updateVariants(int debug_level=0);
	//update GRZ/KDK export status
	void updateExportStatus();
	void updateExportStatus(NGSD& mvh_db, int r);

	//check for errors in the data
	void checkForMetaDataErrors();
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

	//returns a list of causal variants
	enum VarType
	{
		CAUSAL,
		INCIDENTAL,
		VUS
	};
	struct VarData
	{
		QByteArray name;
		QByteArray localization;
		VarType type;
	};
	QList<VarData> getVariants(NGSD& db, QString ps);
	GeneSet variantGenes(NGSD& db, const Chromosome& chr, int start, int end);
	QByteArray variantLocalization(NGSD& db, const Chromosome& chr, int start, int end, const GeneSet& genes);
};

#endif // MVHUB_H
