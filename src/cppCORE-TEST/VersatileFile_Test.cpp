#include "TestFramework.h"
#include "VersatileFile.h"
#include <QNetworkProxyFactory>
#include "Settings.h"

TEST_CLASS(VersatileFile_Test)
{
private:


	TEST_METHOD(local_opening_and_reading)
    {
        VersatileFile file(TESTDATA("data_in/txt_file.txt"));
        I_EQUAL(file.mode(), VersatileFile::LOCAL);

		file.open();

		QString entire_file = file.readAll();
        S_EQUAL(entire_file, "##comment\n#header\nthis is a plain text file");
        IS_TRUE(file.atEnd())
        file.seek(0);

        IS_FALSE(file.atEnd())
        QString line = file.readLine();
        S_EQUAL(line.trimmed(), "##comment");
        I_EQUAL(file.pos(), 10);

        QString fragment = file.read(7);
        S_EQUAL(fragment, "#header");
        I_EQUAL(file.pos(), 17);

        file.seek(0);
        QString start_fragment = file.read(9);
        S_EQUAL(start_fragment, "##comment");
        I_EQUAL(file.pos(), 9);
    }

	TEST_METHOD(local_readAll_gz)
    {
        VersatileFile file(TESTDATA("data_in/txt_file_gzipped.txt.gz"));
        I_EQUAL(file.mode(), VersatileFile::LOCAL_GZ);

		file.open();
		QString entire_file = file.readAll();
		S_EQUAL(entire_file.trimmed(), "##comment\r\n#header\r\nthis is a gzipped text file");
		IS_TRUE(file.atEnd())
    }

	TEST_METHOD(local_readAll_bgz)
	{
		VersatileFile file(TESTDATA("data_in/txt_file_bgzipped.txt.bgz"));
		I_EQUAL(file.mode(), VersatileFile::LOCAL_GZ);

		file.open();
		QString entire_file = file.readAll();
		S_EQUAL(entire_file.trimmed(), "##comment\n#header\nthis is a bzgipped text file");
		IS_TRUE(file.atEnd())
	}

	TEST_METHOD(remote_readAll_plain)
	{
		VersatileFile file("https://raw.githubusercontent.com/imgag/ngs-bits/refs/heads/master/src/cppCORE-TEST/data_in/txt_file.txt");
		I_EQUAL(file.mode(), VersatileFile::URL);

		//at UKT, we need proxy credentials...
		if (file.proxy().hostName().endsWith(".med.uni-tuebingen.de") && Settings::string("proxy_password").trimmed().isEmpty()) return;

		file.open();
		QString entire_file = file.readAll();
		S_EQUAL(entire_file.trimmed(), "##comment\n#header\nthis is a plain text file");
		IS_TRUE(file.atEnd())
	}

	TEST_METHOD(remote_readAll_gz)
	{
		VersatileFile file("https://raw.githubusercontent.com/imgag/ngs-bits/refs/heads/master/src/cppCORE-TEST/data_in/txt_file_gzipped.txt.gz");
		I_EQUAL(file.mode(), VersatileFile::URL_GZ);

		//at UKT, we need proxy credentials...
		if (file.proxy().hostName().endsWith(".med.uni-tuebingen.de") && Settings::string("proxy_password").trimmed().isEmpty()) return;

		file.open();
		QString entire_file = file.readAll();
		S_EQUAL(entire_file.trimmed(), "##comment\r\n#header\r\nthis is a gzipped text file");
		IS_TRUE(file.atEnd())
	}
};