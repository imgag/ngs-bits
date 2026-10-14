#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "TsvFile.h"
#include "TabixIndexedFile.h"
#include "BasicStatistics.h"

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Extracts methylations from a modkit bedmethyl file for a given set of imprinting sites.");
		addInfile("in_hp1", "Input modkit bedmethyl file for first allele (in BED.GZ format).", false);
		addInfile("in_hp2", "Input modkit bedmethyl file for second allele (in BED.GZ format).", false);
		addInfile("in_all", "Input modkit bedmethyl file for combined allele track (in BED.GZ format).", false);
		addInfile("sites", "Input table containing imprinting sites (in TSV format).", false);
		addOutfile("out", "Output table in TSV format.", false);

		//optional
		addOutfile("out_folder", "Output folder to put extracted modkit lines (for later plotting).", true);
		addInfileList("cohort_hp1", "List of bedmethyl cohort files for first allele (in BED.GZ format).", true);
		addInfileList("cohort_hp2", "List of bedmethyl cohort files for second allele (in BED.GZ format).", true);
		addInfileList("cohort_all", "List of bedmethyl cohort files for combined allele track (in BED.GZ format).", true);

	}

	virtual void main()
	{
		// check inputs
		// - input cohort lists same lengths
		QStringList cohort_file_paths_hp1 = getInfileList("cohort_hp1");
		QStringList cohort_file_paths_hp2 = getInfileList("cohort_hp2");
		QStringList cohort_file_paths_all = getInfileList("cohort_all");

		if ((cohort_file_paths_hp1.size() != cohort_file_paths_hp2.size()) || (cohort_file_paths_hp1.size() != cohort_file_paths_all.size()))
		{
			THROW(ArgumentException, "List of cohort BED files have to have the same length for each allele!");
		}
		bool cohort_annotation = (cohort_file_paths_hp1.size() > 0);

		//parse table
		TsvFile imprinting_sites;
		imprinting_sites.load(getInfile("sites"));
		// int idx_name = imprinting_sites.columnIndex("identifier");
		int idx_chr = imprinting_sites.columnIndex("gene chr");
		int idx_highlight_start = imprinting_sites.columnIndex("highlight start");
		int idx_highlight_end = imprinting_sites.columnIndex("highlight end");
		// int idx_plot_start = imprinting_sites.columnIndex("gene start");
		// int idx_plot_end = imprinting_sites.columnIndex("gene end");


		//parse input files
		QByteArrayList prefixes = QByteArrayList() << "hp1" << "hp2" << "all";
		QMap<QByteArray, TabixIndexedFile> bed_files;
		bed_files["hp1"].load(getInfile("in_hp1").toUtf8());
		bed_files["hp2"].load(getInfile("in_hp2").toUtf8());
		bed_files["all"].load(getInfile("in_all").toUtf8());

		//init output file
		QString out_path = getOutfile("out");
		TsvFile output;
		output.addComment("##DESCRIPTION=meth_hp1_avg=average methylation in haplotype 1 (higher mean methylation)");
		output.addComment("##DESCRIPTION=meth_hp1_std=standard deviation of methylation in haplotype 1 (higher mean methylation)");
		output.addComment("##DESCRIPTION=cov_hp1=average CpG coverage in haplotype 1 (higher mean methylation)");
		output.addComment("##DESCRIPTION=meth_hp2_avg=average methylation in haplotype 2 (lower mean methylation)");
		output.addComment("##DESCRIPTION=meth_hp2_std=standard deviation of methylation in haplotype 2 (lower mean methylation)");
		output.addComment("##DESCRIPTION=cov_hp2=average CpG coverage in haplotype 2 (lower mean methylation)");
		output.addComment("##DESCRIPTION=meth_all_avg=average methylation");
		output.addComment("##DESCRIPTION=meth_all_std=standard deviation of methylation");
		output.addComment("##DESCRIPTION=cov_all=average CpG coverage");
		output.addHeader("identifier");
		output.addHeader("title");
		output.addHeader("gene symbol");
		output.addHeader("gene chr");
		output.addHeader("gene start");
		output.addHeader("gene end");
		output.addHeader("highlight start");
		output.addHeader("highlight end");
		output.addHeader("filter");
		output.addHeader("meth_hp1_avg");
		output.addHeader("meth_hp1_std");
		output.addHeader("cov_hp1");
		output.addHeader("meth_hp2_avg");
		output.addHeader("meth_hp2_std");
		output.addHeader("cov_hp2");
		output.addHeader("meth_all_avg");
		output.addHeader("meth_all_std");
		output.addHeader("cov_all");


		QList<QMap<QByteArray, TabixIndexedFile>> cohort_bed_files;
		if (cohort_annotation)
		{
			// add additional columns
			output.addHeader("cohort_mean_hp1");
			output.addHeader("cohort_stdev_hp1");
			output.addHeader("zscore_hp1");
			output.addHeader("cohort_mean_hp2");
			output.addHeader("cohort_stdev_hp2");
			output.addHeader("zscore_hp2");
			output.addHeader("cohort_mean_all");
			output.addHeader("cohort_stdev_all");
			output.addHeader("zscore_all");
			output.addHeader("cohort_size");

			// open tabix files for cohort samples
			cohort_bed_files.fill(QMap<QByteArray, TabixIndexedFile>(), cohort_file_paths_hp1.size());
			//iterate over cohort
			for (int s = 0; s < cohort_file_paths_all.size(); ++s)
			{
				cohort_bed_files[s]["hp1"].load(cohort_file_paths_hp1[s].toUtf8());
				cohort_bed_files[s]["hp2"].load(cohort_file_paths_hp2[s].toUtf8());
				cohort_bed_files[s]["all"].load(cohort_file_paths_all[s].toUtf8());
			}
		}



		//open tabix indices

		//iterate over table
		for (int i = 0; i < imprinting_sites.count(); ++i)
		{
			//get methylation
			const QStringList& line = imprinting_sites[i];
			Chromosome chr(line.at(idx_chr));
			int start = Helper::toInt(line.at(idx_highlight_start), "highlight start", line.join(" "));
			int end = Helper::toInt(line.at(idx_highlight_end), "highlight end", line.join(" "));


			QMap<QByteArray,double> sample_stats;
			// extract methylation
			for (const QByteArray& prefix : std::as_const(prefixes))
			{
				QByteArrayList lines = bed_files[prefix].getMatchingLines(chr, start, end);
				sample_stats.insert(averageMethylation(lines, prefix));
			}

			// sort haplotypes
			sortHaplotypes(sample_stats);

			//TODO: check phasing blocks

			//TODO: fill filter column
			QByteArrayList filter;

			//check fraction of unphased reads
			double unphased_fraction = (sample_stats["hp1_cov"] + sample_stats["hp2_cov"]) /sample_stats["hp1_all"];
			//TODO: adjust threshold
			if (unphased_fraction < 0.5) filter << "HighlyUnphasedRegion";


			// add cohort statistics
			QMap<QByteArray, double> cohort_stats;
			if (cohort_annotation)
			{
				QMap<QByteArray, QList<double>> means;
				// QList<double> coverages;
				//iterate over cohort
				for (int s = 0; s < cohort_file_paths_all.size(); ++s)
				{
					QMap<QByteArray,double> cohort_sample_stats;
					for (const QByteArray& prefix : std::as_const(prefixes))
					{
						//extract methylation
						QByteArrayList lines = cohort_bed_files[s][prefix].getMatchingLines(chr, start, end);
						// calaculate sample statistics
						cohort_sample_stats.insert(averageMethylation(lines, prefix));
					}
					sortHaplotypes(cohort_sample_stats);

					for (const QByteArray& prefix : std::as_const(prefixes))
					{
						//store mean/coverage values
						if (!std::isnan(cohort_sample_stats[prefix + "_avg"])) means[prefix] << cohort_sample_stats[prefix + "_avg"];


						// TODO: ( write to output folder)
					}


				}
				for (const QByteArray& prefix : std::as_const(prefixes))
				{
					// perform statistics
					cohort_stats["mean_" + prefix] = NAN;
					cohort_stats["stdev_" + prefix] = NAN;
					cohort_stats["zscore_" + prefix] = NAN;
					cohort_stats["cohort_size_" + prefix] = 0.0;
					if (means[prefix].size() > 0)
					{
						cohort_stats["cohort_size_" + prefix] = means[prefix].size();
						cohort_stats["mean_" + prefix] = BasicStatistics::mean(means[prefix]);
						cohort_stats["stdev_" + prefix] = BasicStatistics::stdev(means[prefix], cohort_stats["mean_" + prefix]);
						if (cohort_stats["stdev_" + prefix] != 0.0)
						{
							cohort_stats["zscore_" + prefix] = (sample_stats[prefix + "_avg"] - cohort_stats["mean_" + prefix]) / cohort_stats["stdev_" + prefix];
						}
					}
				}

			}
			// add methylation columns
			output.addRow(assembleImprintingLine(line, filter, sample_stats, cohort_stats));
		}
		//write output
		output.store(out_path);

	}
private:
	QMap<QByteArray,double> averageMethylation(const QByteArrayList& methylation_lines, const QByteArray& prefix)
	{
		//const bedmethyl indices:
		// const int idx_chr = 0;
		// const int idx_start = 1;
		// const int idx_end = 2;
		const int idx_mod_type = 3;
		const int idx_cov = 9;
		const int idx_frac = 10;
		// const int idx_n_mod = 11;
		// QMap<QByteArray, QPair<int, int>> methylation_unphased;
		QMap<QByteArray,double> return_values;

		//combined
		QList<double> coverage_all;
		QList<double> frac_all;
		foreach (const QByteArray& line, methylation_lines)
		{
			QByteArrayList parts = line.split('\t');
			if (parts.size() < 13) THROW(FileParseException, "Invalid BedMethyl line: '" + line + "'!");
			if (parts.at(idx_mod_type).trimmed() != "m") continue;

			int cov = Helper::toInt(parts.at(idx_cov), "N_cov_valid", line);
			// int n_mod = Helper::toInt(parts.at(idx_n_mod), "N_mod", line);
			double frac = Helper::toDouble(parts.at(idx_frac), "fraction modified", line);
			coverage_all << cov;
			frac_all << frac;
			// methylation_unphased.insert(parts.at(idx_chr) + ":" + parts.at(idx_start) + "-" + parts.at(idx_end), QPair<int, int>(n_mod, cov));
		}
		return_values.insert(prefix + "_avg", (frac_all.size() > 0)? BasicStatistics::mean(frac_all): NAN);
		return_values.insert(prefix + "_std",  (frac_all.size() > 0)? BasicStatistics::stdev(frac_all): NAN);
		return_values.insert(prefix + "_cov", (coverage_all.size() > 0)? BasicStatistics::mean(coverage_all): NAN);

		return return_values;
	}

	void sortHaplotypes(QMap<QByteArray,double>& methylation)
	{
		//hp2 NAN : skip
		if (std::isnan(methylation["hp2_avg"])) return;

		//switch if hp1 is smaller or NAN
		if (std::isnan(methylation["hp1_avg"]) || (methylation["hp1_avg"] < methylation["hp2_avg"]))
		{
			//switch hp1 <> hp2
			double tmp_avg = methylation["hp1_avg"];
			double tmp_std = methylation["hp1_std"];
			double tmp_cov = methylation["hp1_cov"];
			methylation["hp1_avg"] = methylation["hp2_avg"];
			methylation["hp1_std"] = methylation["hp2_std"];
			methylation["hp1_cov"] = methylation["hp2_cov"];
			methylation["hp2_avg"] = tmp_avg;
			methylation["hp2_std"] = tmp_std;
			methylation["hp2_cov"] = tmp_cov;
		}
	}


	QStringList assembleImprintingLine(const QStringList& base_line, const QByteArrayList& filter_column, const QMap<QByteArray, double>& sample_stats, const QMap<QByteArray, double>& cohort_stats)
	{
		QStringList line = base_line;
		line << filter_column.join(",");

		//hp1
		line << QByteArray::number(sample_stats["hp1_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["hp1_std"], 'f', 3);
		line << QByteArray::number(sample_stats["hp1_cov"], 'f', 3);

		//hp2
		line << QByteArray::number(sample_stats["hp2_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["hp2_std"], 'f', 3);
		line << QByteArray::number(sample_stats["hp2_cov"], 'f', 3);

		//all
		line << QByteArray::number(sample_stats["all_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["all_std"], 'f', 3);
		line << QByteArray::number(sample_stats["all_cov"], 'f', 3);

		if (cohort_stats.size() > 0)
		{
			line << QByteArray::number(cohort_stats["mean_hp1"], 'f', 3);
			line << QByteArray::number(cohort_stats["stdev_hp1"], 'f', 3);
			line << QByteArray::number(cohort_stats["zscore_hp1"], 'f', 3);
			// line << QByteArray::number(cohort_stats["cohort_size_hp1"], 'f', 3);
			line << QByteArray::number(cohort_stats["mean_hp2"], 'f', 3);
			line << QByteArray::number(cohort_stats["stdev_hp2"], 'f', 3);
			line << QByteArray::number(cohort_stats["zscore_hp2"], 'f', 3);
			// line << QByteArray::number(cohort_stats["cohort_size_hp2"], 'f', 3);
			line << QByteArray::number(cohort_stats["mean_all"], 'f', 3);
			line << QByteArray::number(cohort_stats["stdev_all"], 'f', 3);
			line << QByteArray::number(cohort_stats["zscore_all"], 'f', 3);
			// line << QByteArray::number(cohort_stats["cohort_size_all"], 'f', 3);
			line << QByteArray::number(cohort_stats["cohort_size_hp1"]) + ","
						+ QByteArray::number(cohort_stats["cohort_size_hp2"]) + ","
						+ QByteArray::number(cohort_stats["cohort_size_all"]);
		}


		return line;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
