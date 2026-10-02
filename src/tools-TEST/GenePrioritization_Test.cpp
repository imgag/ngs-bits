#include "TestFramework.h"
#include "BasicStatistics.h"

TEST_CLASS(GenePrioritization_Test)
{
private:

    TEST_METHOD(test_flooding)
    {
		EXECUTE("GenePrioritization", "-in " + TESTDATA("data_in/GenePrioritization_in.tsv") + " -graph " + TESTDATA("data_in/GenePrioritization_graph.tsv") + " -out out/GenePrioritization_out1.tsv -method flooding");
        IS_TRUE(QFile::exists("out/GenePrioritization_out1.tsv"));
        COMPARE_FILES("out/GenePrioritization_out1.tsv", TESTDATA("data_out/GenePrioritization_out1.tsv"));
    }

	QMap<QByteArray, double> loadGeneScoreMap(QString filename)
	{
		QMap<QByteArray, double> output;

		VersatileFile file(filename);
		file.open(QFile::ReadOnly);
		while(!file.atEnd())
		{
			QByteArray line = file.readLine(true);
			QByteArrayList parts = line.split('\t');

			QByteArray gene_id = parts[0];
			if(!gene_id.startsWith("HGNC:")) continue;

			output.insert(gene_id, Helper::toDouble(parts[1], "gene rank score", line));
		}

		return output;
	}

	QVector<double> getScoreVector(QMap<QByteArray, double> scores, QByteArrayList gene_ids)
	{
		QVector<double> output;

		for (const QByteArray& gene_id: gene_ids)
		{
			double score = scores.value(gene_id, -1.0);
			if (score<0) THROW(FileParseException, "Score for gene with ID " + gene_id + " no found!");
			output << score;
		}

		return output;
	}

    TEST_METHOD(test_random_walk)
    {
		EXECUTE("GenePrioritization", "-in " + TESTDATA("data_in/GenePrioritization_in.tsv") + " -graph " + TESTDATA("data_in/GenePrioritization_graph.tsv") + " -out out/GenePrioritization_out2.tsv -method random_walk");
        IS_TRUE(QFile::exists("out/GenePrioritization_out2.tsv"));

		//comparing files is not possible since that random numbers are differ on different platforms, so we compare the correlation of ranks
		QMap<QByteArray, double> expected = loadGeneScoreMap(TESTDATA("data_out/GenePrioritization_out2.tsv"));
		QMap<QByteArray, double> out = loadGeneScoreMap("out/GenePrioritization_out2.tsv");

		QByteArrayList gene_ids = expected.keys();
		QVector<double> ranks_expected = getScoreVector(expected, gene_ids);
		QVector<double> ranks_out = getScoreVector(out, gene_ids);

		double correlation = BasicStatistics::correlation(ranks_expected, ranks_out);
		IS_TRUE(correlation>0.95);
    }
};
