#include "TestFramework.h"
#include "VersatileTextStream.h"

TEST_CLASS(VersatileTextStream_Test)
{
public:

	void plain_text_file()
	{
		VersatileTextStream stream(TESTDATA("data_in/txt_file.txt"));
		I_EQUAL(stream.mode(), VersatileFile::LOCAL);

		IS_FALSE(stream.atEnd())
		QString line = stream.readLine();
		S_EQUAL(line, "##comment");

		IS_FALSE(stream.atEnd())
		line = stream.readLine();
		S_EQUAL(line, "#header");

		IS_FALSE(stream.atEnd())
		line = stream.readLine();
		S_EQUAL(line, "this is a plain text file");

		IS_TRUE(stream.atEnd())
		line = stream.readLine();
		S_EQUAL(line, "");
	}


	void gzipped_text_file()
	{
		VersatileTextStream stream(TESTDATA("data_in/txt_file_gzipped.txt.gz"));
		I_EQUAL(stream.mode(), VersatileFile::LOCAL_GZ);

		IS_FALSE(stream.atEnd())
		QString line = stream.readLine();
		S_EQUAL(line, "##comment");

		IS_FALSE(stream.atEnd())
		line = stream.readLine();
		S_EQUAL(line, "#header");

		IS_FALSE(stream.atEnd())
		line = stream.readLine();
		S_EQUAL(line, "this is a gzipped text file");

		IS_FALSE(stream.atEnd())
		line = stream.readLine();
		S_EQUAL(line, "");

		IS_TRUE(stream.atEnd())
	}
};
