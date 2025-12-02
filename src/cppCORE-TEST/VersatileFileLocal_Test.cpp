#include "TestFramework.h"
#include "VersatileFile.h"

TEST_CLASS(VersatileFileLocal_Test)
{
private:

    TEST_METHOD(plain_text_file)
    {
        VersatileFile file(TESTDATA("data_in/txt_file.txt"));
        file.open();
        I_EQUAL(file.mode(), VersatileFile::LOCAL);

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


    TEST_METHOD(gzipped_text_file)
    {
        VersatileFile file(TESTDATA("data_in/txt_file_gzipped.txt.gz"));
        file.open();
        I_EQUAL(file.mode(), VersatileFile::LOCAL_GZ);

        QString entire_file = file.readAll();
        QString expected_content = "##comment\r\n#header\r\nthis is a gzipped text file";
        S_EQUAL(entire_file.trimmed(), expected_content);
        IS_TRUE(file.atEnd())
        file.seek(0);

        IS_FALSE(file.atEnd())
        QString line = file.readLine();
        S_EQUAL(line.trimmed(), "##comment");
    }
};
