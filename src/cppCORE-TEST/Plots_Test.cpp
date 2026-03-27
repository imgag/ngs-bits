#include "TestFramework.h"
#include "Histogram.h"

TEST_CLASS(Plots_Test)
{
private:

	TEST_METHOD(combined_histogram)
	{
		Histogram hist_all(0,1,0.0125);
		hist_all.setLabel("all variants");
		QVector<double> y_coords_all = {32, 1495, 581, 251, 179, 349, 505, 457,
					1208, 758, 1953, 1396, 1807, 1799, 2015, 320,
					2203, 2246, 341, 17, 1930, 325, 1747, 14, 324,
					22, 1514, 9, 19, 53, 214, 4, 1251, 3, 203, 40,
					11, 1, 1, 1, 1229, 2, 1, 2, 9, 30, 2, 1, 148,
					0, 14, 0, 0, 936, 1, 2, 3, 14, 3, 0, 144, 0, 6,
					0, 37, 7, 54, 3, 20, 1, 26, 13, 14, 8, 7, 2, 1,
					0, 0, 1050};
		hist_all.setBins(y_coords_all);
		hist_all.setBinSum(31388);

		Histogram hist_filtered(0,1,0.0125);
		hist_filtered.setLabel("variants with filter PASS");
		QVector<double> y_coords_filtered = {0, 0, 0, 0, 39, 28, 19,
					18, 20, 13, 11, 9, 19, 16, 19, 10,
					17, 13, 11, 3, 5, 4, 0, 0, 6, 3, 1,
					4, 6, 1, 1, 1, 4, 0, 4, 1, 1, 0, 0, 0,
					0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 2, 0, 0,
					0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		hist_filtered.setBins(y_coords_filtered);
		hist_filtered.setBinSum(317);

		QString plot_file_name = "combined_histogram_test.png";
		Histogram::storeCombinedHistogram(plot_file_name, QList<Histogram>({hist_all,hist_filtered}),"tumor allele frequency","count");

		IS_TRUE(QFile(plot_file_name).exists());
		IS_TRUE(QFile(plot_file_name).size()>0);
	}
};
