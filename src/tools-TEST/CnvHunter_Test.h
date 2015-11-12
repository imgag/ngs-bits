#include "TestFramework.h"
#include "Settings.h"

TEST_CLASS(CnvHunter_Test)
{
Q_OBJECT
private slots:

	void hpPDv3()
	{
	    QString data_folder = TESTDATA("data_in/CnvHunter1/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out1.txt");
		COMPARE_FILES("out/CnvHunter_out1.txt", TESTDATA("data_out/CnvHunter_out1.txt"));
	}

	void hpSCv1()
	{
	    QString data_folder = TESTDATA("data_in/CnvHunter2/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out2.txt");
		COMPARE_FILES("out/CnvHunter_out2.txt", TESTDATA("data_out/CnvHunter_out2.txt"));
	}
	
	void ssX()
	{
		QString data_folder = TESTDATA("data_in/CnvHunter3/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out3.txt");
		COMPARE_FILES("out/CnvHunter_out3.txt", TESTDATA("data_out/CnvHunter_out3.txt"));
	}

	void hpSCAv4_excludeReg_regionBedFile()
	{
		QString data_folder = TESTDATA("data_in/CnvHunter4/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out4.txt -out_reg out/CnvHunter_out4.bed -exclude " +  TESTDATA("data_in/CnvHunter4/excluded.bed"));
		COMPARE_FILES("out/CnvHunter_out4.txt", TESTDATA("data_out/CnvHunter_out4.txt"));
	}

	void ssKM()
	{
		QString data_folder = TESTDATA("data_in/CnvHunter5/");
		QStringList in = QDir(data_folder).entryList(QStringList() << "*.cov");
		for (int i=0; i<in.count(); ++i)
		{
			in[i] = data_folder + in[i];
		}
		EXECUTE("CnvHunter", "-in " + in.join(" ") + " -out out/CnvHunter_out5.txt");
		COMPARE_FILES("out/CnvHunter_out5.txt", TESTDATA("data_out/CnvHunter_out5.txt"));
	}
};
