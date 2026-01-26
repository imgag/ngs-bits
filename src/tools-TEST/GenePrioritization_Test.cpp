#include "TestFramework.h"


TEST_CLASS(GenePrioritization_Test)
{
private:

    TEST_METHOD(test_flooding)
    {
        EXECUTE("GenePrioritization", "-in " + TESTDATA("data_in/GenePrioritization_in.tsv") + " -graph " +
                TESTDATA("data_in/GenePrioritization_graph.tsv") + " -out out/GenePrioritization_out1.tsv" +
                " -method flooding");
        IS_TRUE(QFile::exists("out/GenePrioritization_out1.tsv"));
        COMPARE_FILES("out/GenePrioritization_out1.tsv", TESTDATA("data_out/GenePrioritization_out1.tsv"));
    }

    TEST_METHOD(test_random_walk)
    {
        EXECUTE("GenePrioritization", "-in " + TESTDATA("data_in/GenePrioritization_in.tsv") + " -graph " +
                TESTDATA("data_in/GenePrioritization_graph.tsv") + " -out out/GenePrioritization_out2.tsv" +
				" -method random_walk");
        IS_TRUE(QFile::exists("out/GenePrioritization_out2.tsv"));

        // random number generator behaves differently under different operating systems
        if(Helper::isMacOS())
        {
            COMPARE_FILES("out/GenePrioritization_out2.tsv", TESTDATA("data_out/GenePrioritization_out2_OSX.tsv"));
        }
        else
        {
            COMPARE_FILES("out/GenePrioritization_out2.tsv", TESTDATA("data_out/GenePrioritization_out2.tsv"));
        }
    }
};
