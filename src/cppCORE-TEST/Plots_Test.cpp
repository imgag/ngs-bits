#include "TestFramework.h"
#include "Histogram.h"
#include "LinePlot.h"
#include "ScatterPlot.h"
#include "BarPlot.h"

TEST_CLASS(Plots_Test)
{
private:	
	TEST_METHOD(create_lineplot)
	{
		QVector<double> x_coords = {10, 15, 20, 25, 40};
		QVector<double> y_coords = {7, 7, 7, 10, 35};

		LinePlot test_plot;
		test_plot.setXLabel("x axis label");
		test_plot.setYLabel("y axis label");
		test_plot.setXValues(x_coords);
		test_plot.addLine(y_coords);

		QString plot_file_name = "out/lineplot_test.png";
		test_plot.store(plot_file_name);

		IS_TRUE(QFile(plot_file_name).exists());
		IS_TRUE(QFile(plot_file_name).size()>0);

		COMPARE_PNG_FILES(plot_file_name, TESTDATA("data_out/lineplot_test.png"), 0.92, Qt::white);
	}

	TEST_METHOD(create_scatterplot)
	{
		ScatterPlot test_plot;
		test_plot.setXLabel("tumor allele frequency");
		test_plot.setYLabel("normal allele frequency");
		test_plot.setXRange(0,1.015);
		test_plot.setYRange(0,1.015);

		QList<std::pair<double,double>> points;
		points.append(std::pair<double, double>(0.1, 0.2));
		points.append(std::pair<double, double>(0.3, 0.3));
		points.append(std::pair<double, double>(0.4, 0.2));
		points.append(std::pair<double, double>(0.5, 0.4));
		points.append(std::pair<double, double>(0.7, 0.45));
		points.append(std::pair<double, double>(0.9, 0.8));

		QList<QString> colors({"blue","black","red","green","cyan","yellow"});
		test_plot.setValues(points, colors);

		QString plot_file_name = "out/scatterplot_test.png";
		test_plot.store(plot_file_name);

		IS_TRUE(QFile(plot_file_name).exists());
		IS_TRUE(QFile(plot_file_name).size()>0);

		COMPARE_PNG_FILES(plot_file_name, TESTDATA("data_out/scatterplot_test.png"), 0.92, Qt::white);
	}

	TEST_METHOD(create_regular_histogram)
	{
		Histogram regualar_histogram(0,247.795,12.3897);
		regualar_histogram.setBinSum(3);
		QVector<double> y_coords = {0, 0, 0,
									0, 0, 1,
									0, 0, 0,
									0, 1, 0,
									0, 0, 0,
									0, 0, 0,
									0, 1};
		regualar_histogram.setBins(y_coords);
		regualar_histogram.setXLabel("phasing block size (kb)");
		regualar_histogram.setYLabel("count");
		regualar_histogram.setLabel("normal histogram");

		QString plot_file_name = "out/regular_histogram_test.png";
		regualar_histogram.store(plot_file_name, false, true, 0.5);

		IS_TRUE(QFile(plot_file_name).exists());
		IS_TRUE(QFile(plot_file_name).size()>0);

		COMPARE_PNG_FILES(plot_file_name, TESTDATA("data_out/regular_histogram_test.png"), 0.92, Qt::white);
	}

	TEST_METHOD(create_combined_histogram)
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

		QString plot_file_name = "out/combined_histogram_test.png";
		Histogram::storeCombinedHistogram(plot_file_name, QList<Histogram>({hist_all,hist_filtered}),"tumor allele frequency","count");

		IS_TRUE(QFile(plot_file_name).exists());
		IS_TRUE(QFile(plot_file_name).size()>0);

		COMPARE_PNG_FILES(plot_file_name, TESTDATA("data_out/combined_histogram_test.png"), 0.92, Qt::white);
	}

	TEST_METHOD(create_barplot)
	{
		QList<int> counts({0,0,1,0,0,0});
		QList<QString> nuc_changes({"C>A","C>G","C>T","T>A","T>G","T>C"});
		QList<QString> colors({"blue","black","red","green","cyan","yellow"});
		int ymax = 1;

		BarPlot test_plot;
		test_plot.setXLabel("base change");
		test_plot.setYLabel("count");
		test_plot.setYRange(0,ymax*1.2);
		test_plot.setXRange(-1.5,nuc_changes.count()+0.5);
		test_plot.setValues(counts, nuc_changes, colors);

		QString plot_file_name = "out/barplot_test.png";
		test_plot.store(plot_file_name);

		IS_TRUE(QFile(plot_file_name).exists());
		IS_TRUE(QFile(plot_file_name).size()>0);


		QList<int> frequencies({1,4,7,0,0,0,0,0,0,0,
								0,0,0,0,0,10,0,0,0,0,
								0,0,0,0,0,0,0,0,0,0,
								0,0,100,0,0,50,0,0,0,
								0,0,0,0,0,0,0,0,0,0,
								0,0,0,0,0,0,20,0,0,0,
								0,0,0,30,0,0,0,0,0,0,
								0,0,0,0,45,0,0,0,0,0,
								0,0,0,11,0,0,0,50,0,0,
								0,0,0,0,0,0,78});
		QList<QString> labels({"ACA","ACC","ACG","ACT","CCA","CCC","CCG","CCT","GCA",
								"GCC","GCG","GCT","TCA","TCC","TCG","TCT","ACA","ACC",
								"ACG","ACT","CCA","CCC","CCG","CCT","GCA","GCC","GCG",
								"GCT","TCA","TCC","TCG","TCT","ACA","ACC","ACG","ACT",
								"CCA","CCC","CCG","CCT","GCA","GCC","GCG","GCT","TCA",
								"TCC","TCG","TCT","ATA","ATC","ATG","ATT","CTA","CTC",
								"CTG","CTT","GTA","GTC","GTG","GTT","TTA","TTC","TTG",
								"TTT","ATA","ATC","ATG","ATT","CTA","CTC","CTG","CTT",
								"GTA","GTC","GTG","GTT","TTA","TTC","TTG","TTT","ATA",
								"ATC","ATG","ATT","CTA","CTC","CTG","CTT","GTA","GTC",
								"GTG","GTT","TTA","TTC","TTG","TTT"});
		colors = {"blue","blue","blue","blue","blue","blue","blue","blue","blue","blue",
				"blue","blue","blue","blue","blue","blue","black","black","black","black",
				"black","black","black","black","black","black","black","black","black",
				"black","black","black","red","red","red","red","red","red","red","red",
				"red","red","red","red","red","red","red","red","green","green","green",
				"green","green","green","green","green","green","green","green","green",
				"green","green","green","green","yellow","yellow","yellow","yellow","yellow",
				"yellow","yellow","yellow","yellow","yellow","yellow","yellow","yellow",
				"yellow","yellow","yellow","cyan","cyan","cyan","cyan","cyan","cyan",
				"cyan","cyan","cyan","cyan","cyan","cyan","cyan","cyan", "cyan","cyan"};
		ymax = 100;

		BarPlot test_plot2;
		test_plot2.setXLabel("x axis");
		test_plot2.setYLabel("count");
		test_plot2.setYRange(0,ymax*1.2);
		test_plot2.setXRange(-1.5,frequencies.count()+0.5);
		test_plot2.setValues(frequencies, labels, colors);

		QString plot_file_name2 = "out/barplot_test2.png";
		test_plot2.store(plot_file_name2);

		IS_TRUE(QFile(plot_file_name2).exists());
		IS_TRUE(QFile(plot_file_name2).size()>0);

		COMPARE_PNG_FILES(plot_file_name2, TESTDATA("data_out/barplot_test2.png"), 0.92, Qt::white);
	}
};
