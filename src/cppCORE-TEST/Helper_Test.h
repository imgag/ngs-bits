#include "TestFramework.h"
#include "Helper.h"

TEST_CLASS(Helper_Test)
{
Q_OBJECT
private slots:

	void randomNumber()
	{
		for (int i=0; i<1000; ++i)
		{
			double random = Helper::randomNumber(-1, 1);
			IS_TRUE(random>=-1);
			IS_TRUE(random<=1);
		}
	}

	void randomString()
	{
		I_EQUAL(Helper::randomString(8).count(), 8);
        QString bases = Helper::randomString(1000, "ACGT");
        I_EQUAL(bases.count(), 1000);
        IS_TRUE(bases.count('A')>0);
        IS_TRUE(bases.count('C')>0);
        IS_TRUE(bases.count('G')>0);
        IS_TRUE(bases.count('T')>0);
    }

	void loadTextFile()
	{
		QStringList genes;
		genes = Helper::loadTextFile(TESTDATA("data_in/genes.txt"), true, '#', true);
		S_EQUAL(genes[0], QString("BRAF"));
		S_EQUAL(genes[1], QString("KRAS"));
		S_EQUAL(genes[2], QString("BRCA1"));
		S_EQUAL(genes[3], QString("BRCA2"));
		I_EQUAL(genes.count(), 4);

		genes = Helper::loadTextFile(TESTDATA("data_in/genes.txt"), false, '#', true);
		S_EQUAL(genes[0], QString("BRAF"));
		S_EQUAL(genes[1], QString("KRAS"));
		S_EQUAL(genes[2], QString(" "));
		S_EQUAL(genes[3], QString("BRCA1"));
		S_EQUAL(genes[4], QString("BRCA2"));
		I_EQUAL(genes.count(), 5);

		genes = Helper::loadTextFile(TESTDATA("data_in/genes.txt"), true, QChar::Null, false);
		S_EQUAL(genes[0], QString("#header"));
		S_EQUAL(genes[1], QString("BRAF"));
		S_EQUAL(genes[2], QString("KRAS"));
		S_EQUAL(genes[3], QString(""));
		S_EQUAL(genes[4], QString("BRCA1"));
		S_EQUAL(genes[5], QString("BRCA2"));
		I_EQUAL(genes.count(), 6);
	}

	void storeTextFile()
	{
		QStringList genes;
		genes << "#header" << "Gene1" << "Gene2" << "Gene3";
		Helper::storeTextFile("out/Helper_storeTextFile_out.txt", genes);
		COMPARE_FILES("out/Helper_storeTextFile_out.txt", TESTDATA("data_out/Helper_storeTextFile_out.txt"));
	}

	void levenshtein()
	{
		I_EQUAL(Helper::levenshtein("", ""), 0);
		I_EQUAL(Helper::levenshtein("abc", "abc"), 0);

		I_EQUAL(Helper::levenshtein("abc", "xbc"), 1);
		I_EQUAL(Helper::levenshtein("abc", "axc"), 1);
		I_EQUAL(Helper::levenshtein("abc", "abx"), 1);
		I_EQUAL(Helper::levenshtein("xbc", "abc"), 1);
		I_EQUAL(Helper::levenshtein("axc", "abc"), 1);
		I_EQUAL(Helper::levenshtein("abx", "abc"), 1);

		I_EQUAL(Helper::levenshtein("abc", "a"), 2);
		I_EQUAL(Helper::levenshtein("abc", "b"), 2);
		I_EQUAL(Helper::levenshtein("abc", "c"), 2);
		I_EQUAL(Helper::levenshtein("a", "abc"), 2);
		I_EQUAL(Helper::levenshtein("b", "abc"), 2);
		I_EQUAL(Helper::levenshtein("c", "abc"), 2);

		I_EQUAL(Helper::levenshtein("", "abc"), 3);
		I_EQUAL(Helper::levenshtein("abc", ""), 3);
	}

	void findFiles()
	{
		QStringList files = Helper::findFiles(TESTDATA(""), "*.txt", false);
		I_EQUAL(files.count(), 0);
		files = Helper::findFiles(TESTDATA(""), "*.txt", true);
		IS_TRUE(files.count()>0);
		files = Helper::findFiles(TESTDATA(""), "*.blabla", true);
		I_EQUAL(files.count(), 0);
	}

	void findFolder()
	{
		QStringList folders = Helper::findFolders(TESTDATA(""), "data_*", false);
		I_EQUAL(folders.count(), 2);
		folders = Helper::findFolders(TESTDATA(""), "data_*", true);
		I_EQUAL(folders.count(), 2);
		folders = Helper::findFolders(TESTDATA(""), "blabla_*", true);
		I_EQUAL(folders.count(), 0);
	}

	void canonicalPath()
	{
		if (Helper::isWindows())
		{
			S_EQUAL(Helper::canonicalPath("C:/file.txt"), "C:\\file.txt");
			S_EQUAL(Helper::canonicalPath("C:\\file.txt"), "C:\\file.txt");
			S_EQUAL(Helper::canonicalPath("C://folder//file.bed"), "C:\\folder\\file.bed");
			S_EQUAL(Helper::canonicalPath("C://.//file.bed"), "C:\\file.bed");
			S_EQUAL(Helper::canonicalPath("C://ignore_me//../folder/file.bed"), "C:\\folder\\file.bed");
			S_EQUAL(Helper::canonicalPath("C://ignore_me//..//ignore_me2//../folder/file.bed"), "C:\\folder\\file.bed");
			S_EQUAL(Helper::canonicalPath("\\\\some-server\\\\some-share\\some_file.txt"), "\\\\some-server\\some-share\\some_file.txt");
		}
		else //Unix
		{
			S_EQUAL(Helper::canonicalPath("/users/bioinf/file.txt"), "/users/bioinf/file.txt");
			S_EQUAL(Helper::canonicalPath("/users/./bioinf/file.txt"), "/users/bioinf/file.txt");
			S_EQUAL(Helper::canonicalPath("/users/klaus/../bioinf/file.txt"), "/users/bioinf/file.txt");
		}
	}

	void FormatLargeNumber()
	{
		//raw counts
		S_EQUAL(Helper::formatLargeNumber(123, "raw_counts"), "123");
		S_EQUAL(Helper::formatLargeNumber(12345, "raw_counts"), "12345");
		S_EQUAL(Helper::formatLargeNumber(12345678, "raw_counts"), "12345678");
		S_EQUAL(Helper::formatLargeNumber(1234567890, "raw_counts"), "1234567890");

		//modifier
		S_EQUAL(Helper::formatLargeNumber(123, "modifier"), "123");
		S_EQUAL(Helper::formatLargeNumber(12345, "modifier"), "12.35 k");
		S_EQUAL(Helper::formatLargeNumber(12345678, "modifier"), "12.35 M");
		S_EQUAL(Helper::formatLargeNumber(123456789012, "modifier"), "123.46 G");
		S_EQUAL(Helper::formatLargeNumber(12345678901234, "modifier"), "12.35 T");

		//thousands separator
		S_EQUAL(Helper::formatLargeNumber(123, "thousands_separator"), "123");
		S_EQUAL(Helper::formatLargeNumber(12345, "thousands_separator"), "12,345");
		S_EQUAL(Helper::formatLargeNumber(12345678, "thousands_separator"), "12,345,678");
		S_EQUAL(Helper::formatLargeNumber(1234567890, "thousands_separator"), "1,234,567,890");
		S_EQUAL(Helper::formatLargeNumber(123456789012, "thousands_separator"), "123,456,789,012");
	}


	void isNumeric()
	{
		//raw counts
		IS_TRUE(Helper::isNumeric(QString("-10")));
		IS_TRUE(Helper::isNumeric(QString("0")));
		IS_TRUE(Helper::isNumeric(QString("9")));
		IS_TRUE(Helper::isNumeric(QString("1000")));
		IS_TRUE(Helper::isNumeric(QString("-10.4494")));
		IS_TRUE(Helper::isNumeric(QString("0.443")));
		IS_TRUE(Helper::isNumeric(QString("9.146")));
		IS_TRUE(Helper::isNumeric(QString("1000.414")));

		IS_FALSE(Helper::isNumeric(QString("--10.4494")));
		IS_FALSE(Helper::isNumeric(QString("0.443s")));
		IS_FALSE(Helper::isNumeric(QString("abc")));
		IS_FALSE(Helper::isNumeric(QString("10.00.414")));
		IS_FALSE(Helper::isNumeric(QString(" 10.0")));
		IS_FALSE(Helper::isNumeric(QString("10.0 ")));

		IS_TRUE(Helper::isNumeric(QByteArray("-10")));
		IS_TRUE(Helper::isNumeric(QByteArray("0")));
		IS_TRUE(Helper::isNumeric(QByteArray("9")));
		IS_TRUE(Helper::isNumeric(QByteArray("1000")));
		IS_TRUE(Helper::isNumeric(QByteArray("-10.4494")));
		IS_TRUE(Helper::isNumeric(QByteArray("0.443")));
		IS_TRUE(Helper::isNumeric(QByteArray("9.146")));
		IS_TRUE(Helper::isNumeric(QByteArray("1000.414")));

		IS_FALSE(Helper::isNumeric(QByteArray("--10.4494")));
		IS_FALSE(Helper::isNumeric(QByteArray("0.443s")));
		IS_FALSE(Helper::isNumeric(QByteArray("abc")));
		IS_FALSE(Helper::isNumeric(QByteArray("10.00.414")));
		IS_FALSE(Helper::isNumeric(QByteArray("")));
		IS_FALSE(Helper::isNumeric(QByteArray(" 10.0")));
		IS_FALSE(Helper::isNumeric(QByteArray("10.0 ")));
	}

	void mkdir()
	{
		QString base_path = QFileInfo(Helper::tempFileName(".tmp")).path() + QDir::separator();
		QString name;
		while(name.isEmpty() || QFile::exists(base_path+name))
		{
			name = Helper::randomString(20);
		}

		Helper::mkdir(base_path+name);

		IS_TRUE(QFile::exists(base_path+name));

		QDir(base_path).rmdir(name);
	}


	void set777()
	{
		QString base_path = QFileInfo(Helper::tempFileName(".tmp")).path() + QDir::separator();
		QString name;
		while(name.isEmpty() || QFile::exists(base_path+name))
		{
			name = Helper::randomString(20);
		}

		Helper::mkdir(base_path+name);
		bool ok = Helper::set777(base_path+name);
		IS_TRUE(ok);

		QDir(base_path).rmdir(name);
	}

	void executeCommand()
	{
		QString command = "/bin/sh";
		QStringList args =  QStringList() << "-c" << "'echo ok'";
		if (Helper::isWindows())
		{
			command = "cmd.exe";
			args = QStringList() << "/c" << "echo" << "ok";
		}

		QByteArrayList output;
		int exit_code = Helper::executeCommand(command, args, &output);

		I_EQUAL(exit_code, 0);
		S_EQUAL(output.join("").trimmed(), "ok");
	}
};
