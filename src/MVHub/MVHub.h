#ifndef MVHUB_H
#define MVHUB_H

#include <QMainWindow>
#include "DelayedInitializationTimer.h"
#include "ui_MVHub.h"
#include "NGSD.h"
#include "GenLabDB.h"

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
	void openExportHistory(int row);
	void emailExportFailed(int row);
	void updateTableFilters();
	//returns TAN for the given context. Set skip_pseudo1=true of you want to create donor pseudonyms.
	QByteArray getTAN(QByteArray str, QByteArray context, bool skip_pseudo1=false, bool test_server=false, bool debug=false);
	//load research consent data from meDIC
	void loadConsentData();
	//export consent data for a sample list
	void exportConsentData();
	//check XML data in MVH database
	void checkXML();
	//check PS data is up-to-date
	void checkPS();

	//main menu slots
	void on_actionReloadData_triggered();
	void on_actionReloadExportStatus_triggered();
	void on_actionAbout_triggered();

private:
	Ui::MVHub ui_;
	DelayedInitializationTimer delayed_init_;
	QHash<QString, QStringList> cmid2messages_;

	//clear output panel on bottom
	void addOutputHeader(QString section, bool clear=true);
	//add output line and make sure the new line is visible to the user
	void addOutputLine(QString line);

	//load data Modellvorhaben case management RedCap and copy it to MVH database
	void loadDataFromCM(int debug_level=0);
	//load data Modellvorhaben SE RedCap and copy it to MVH database
	void loadDataFromSE();
	//determine processed samples for cases from NGSD
	void determineProcessedSamples(int debug_level=0);
	//add missing HPO terms to SE RedCap and update SE data in MVH database
	int updateHpoTerms(int debug_level=0);
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
	QByteArray getConsent(QString sap_id, bool debug=false);
	//parse JSON and convert it to XML
	QByteArray consentJsonToXml(QByteArray json_text, bool debug=false);

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

	enum Network
	{
		SE,
		OE,
		FBREK,
		UNSET
	};
	Network getNetwork(int row);
	QString networkToString(Network network);

	struct PSData
	{
		QStringList germline;
		QStringList tumor;
	};
	PSData getMatchingPS(NGSD& db, GenLabDB& genlab, QString sap_id, Network network, QString seq_type);


	//returns the row index of the given CM ID
	int rowOf(QString cm_id);
	//returns the columd index of the given column name
	int colOf(QString col, bool throw_if_not_found=true);
};

#endif // MVHUB_H
