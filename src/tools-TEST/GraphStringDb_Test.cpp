#include "TestFramework.h"


TEST_CLASS(GraphStringDb_Test)
{
private:

    TEST_METHOD(create_graph)
    {
        EXECUTE("GraphStringDb", "-string " + TESTDATA("data_in/GraphStringDb_in.txt") + " -alias " +
                TESTDATA("data_in/GraphStringDb_alias.tsv") + " -out out/GraphStringDb_out.tsv");
        IS_TRUE(QFile::exists("out/GraphStringDb_out.tsv"));
        COMPARE_FILES("out/GraphStringDb_out.tsv", TESTDATA("data_out/GraphStringDb_out.tsv"));
    }

};
