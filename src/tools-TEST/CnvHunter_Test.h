#include "TestFramework.h"
#include "Settings.h"

class CnvHunter_Test
		: public QObject
{
	Q_OBJECT

private slots:

	//test1: HaloPlex PD v3 panel with gene annotation
	void test_01()
	{
		QString gene_db = Settings::string("kgxref_merged");
		if (gene_db=="") QSKIP("Test needs a database file!");

	    QString data_folder = QFINDTESTDATA("data_in/CnvHunter1/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		TFW_EXEC("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out1.txt -genes " + gene_db);
		TFW::comareFiles("out/CnvHunter_out1.txt", QFINDTESTDATA("data_out/CnvHunter_out1.txt"));
	}
	
	//test2: Haloplex SCA v1 panel
	void test_02()
	{
	    QString data_folder = QFINDTESTDATA("data_in/CnvHunter2/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		TFW_EXEC("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out2.txt");
		TFW::comareFiles("out/CnvHunter_out2.txt", QFINDTESTDATA("data_out/CnvHunter_out2.txt"));
	}
	
	//test3: SureSelect X panel
	void test_03()
	{
		QString data_folder = QFINDTESTDATA("data_in/CnvHunter3/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		TFW_EXEC("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out3.txt");
		TFW::comareFiles("out/CnvHunter_out3.txt", QFINDTESTDATA("data_out/CnvHunter_out3.txt"));
	}

	//test4: Haloplex SCA v4 panel - excluded regions, regions bed file
	void test_04()
	{
		QString data_folder = QFINDTESTDATA("data_in/CnvHunter4/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		TFW_EXEC("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out4.txt -out_reg out/CnvHunter_out4.bed -exclude " +  QFINDTESTDATA("data_in/CnvHunter4/excluded.bed"));
		TFW::comareFiles("out/CnvHunter_out4.txt", QFINDTESTDATA("data_out/CnvHunter_out4.txt"));
	}

	//test5: SureSelect Kingsmore panel
	void test_05()
	{
		QString data_folder = QFINDTESTDATA("data_in/CnvHunter5/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		TFW_EXEC("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out5.txt");
		TFW::comareFiles("out/CnvHunter_out5.txt", QFINDTESTDATA("data_out/CnvHunter_out5.txt"));
	}

};

TFW_DECLARE(CnvHunter_Test)

