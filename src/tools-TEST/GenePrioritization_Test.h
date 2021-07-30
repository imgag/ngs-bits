#include "TestFramework.h"


TEST_CLASS(GenePrioritization_Test)
{
Q_OBJECT
private slots:

    void test_flooding()
    {
        EXECUTE("GenePrioritization", "-in " + TESTDATA("data_in/GenePrioritization_in.tsv") + " -graph " +
                TESTDATA("data_in/GenePrioritization_graph.tsv") + " -out out/GenePrioritization_out1.tsv" +
                " -method flooding");
        IS_TRUE(QFile::exists("out/GenePrioritization_out1.tsv"));
        COMPARE_FILES("out/GenePrioritization_out1.tsv", TESTDATA("data_out/GenePrioritization_out1.tsv"));
    }

    void test_random_walk()
    {
        EXECUTE("GenePrioritization", "-in " + TESTDATA("data_in/GenePrioritization_in.tsv") + " -graph " +
                TESTDATA("data_in/GenePrioritization_graph.tsv") + " -out out/GenePrioritization_out2.tsv" +
                " -method randomWalk");
        IS_TRUE(QFile::exists("out/GenePrioritization_out2.tsv"));
        COMPARE_FILES("out/GenePrioritization_out2.tsv", TESTDATA("data_out/GenePrioritization_out2.tsv"));
    }
};
