#include "TestFramework.h"
#include "LinePlot.h"
#include "ScatterPlot.h"
#include "BarPlot.h"

TEST_CLASS(PlotFiles_Test)
{
	private:

	TEST_METHOD(line_plot_exists)
	{
		LinePlot line_plot;
		line_plot.setXLabel("insert size");
		line_plot.setYLabel("reads [%]");
		QVector<double> x_values;
		x_values << 0.5 << 1 << 1.2 << 1.7 << 1.8;
		line_plot.setXValues(x_values);
		QVector<double> line_values;
		line_values << 0.0 << 0.7 << 1.1 << 1.3 << 1.9;
		line_plot.addLine(line_values, "label text");

		QString plotname = Helper::tempFileName(".png");
		line_plot.store(plotname);

		IS_TRUE(QFile::exists(plotname));
		IS_TRUE(QFile(plotname).size()>0);
	}

	TEST_METHOD(scatter_plot_exists)
	{
		ScatterPlot scatter_plot;
		scatter_plot.setXLabel("tumor allele frequency");
		scatter_plot.setYLabel("normal allele frequency");
		scatter_plot.setXRange(-0.015,1.015);
		scatter_plot.setYRange(-0.015,1.015);

		QList<std::pair<double,double>> points_test;

		points_test << std::pair<double,double>{0.015, -0.01}
					<< std::pair<double,double>{0.035, 0.015}
					<< std::pair<double,double>{0.115, 0.025}
					<< std::pair<double,double>{0.215, 0.315}
					<< std::pair<double,double>{0.515, 0.5}
					<< std::pair<double,double>{0.715, 0.05}
					<< std::pair<double,double>{0.915, -0.01}
					<< std::pair<double,double>{1.015, 0.7};

		QList<QString> colors_test({"green","black","red","green","cyan","yellow", "yellow", "yellow"});
		scatter_plot.setValues(points_test, colors_test);

		QString g_test = "black";
		QString b_test = "green";
		scatter_plot.addColorLegend(g_test, "all variants");
		scatter_plot.addColorLegend(b_test, "variants with filter PASS");

		QString plotname = Helper::tempFileName(".png");
		scatter_plot.store(plotname);

		IS_TRUE(QFile::exists(plotname));
		IS_TRUE(QFile(plotname).size()>0);
	}

	TEST_METHOD(bar_plot_exists)
	{

		QList<int> counts({5,2,10,15,4,7});
		QList<QString> nuc_changes({"C>A","C>G","C>T","T>A","T>G","T>C"});
		QList<QString> colors({"b","k","r","g","c","y"});
		int ymax = 0;
		foreach(int c, counts)
		{
			if(c>ymax)	ymax = c;
		}


		BarPlot plot0b;
		plot0b.setXLabel("base change");
		plot0b.setYLabel("count");
		QMap<QString,QString> color_map = QMap<QString,QString>{{"C>A","b"},{"C>G","k"},{"C>T","r"},{"T>A","g"},{"T>G","c"},{"T>C","y"}};
		foreach(QString color, color_map)
		{
			plot0b.addColorLegend(color,color_map.key(color));
		}


		plot0b.setYRange(-ymax*0.02,ymax*1.2);
		plot0b.setXRange(-1.5,nuc_changes.count()+0.5);
		plot0b.setValues(counts, nuc_changes, colors);
		QString plot0bname = Helper::tempFileName(".png");
		plot0b.store(plot0bname);

	}
};
