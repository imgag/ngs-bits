#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "TsvFile.h"

#include <TabixIndexedFile.h>

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
		addInfile("table", "Input table containing imprinting sites (in TSV format).", false);

		//optional
		addOutfile("out", "Output table in TSV format. If unset, writes to STDOUT.", true);
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
		TsvFile imprinting_table;
		imprinting_table.load(getInfile("table"));
		int idx_name = imprinting_table.columnIndex("identifier");
		int idx_chr = imprinting_table.columnIndex("gene_chr");
		int idx_highlight_start = imprinting_table.columnIndex("highlight start");
		int idx_highlight_end = imprinting_table.columnIndex("highlight end");
		int idx_plot_start = imprinting_table.columnIndex("gene start");
		int idx_plot_end = imprinting_table.columnIndex("gene end");


		//parse input files
		TabixIndexedFile bed_hp1;
		bed_hp1.load(getInfile("in_hp1").toUtf8());
		TabixIndexedFile bed_hp2;
		bed_hp2.load(getInfile("in_hp1").toUtf8());
		TabixIndexedFile bed_all;
		bed_all.load(getInfile("in_hp1").toUtf8());

		//init output file
		TsvFile output;
		output.addComment("##DESCRIPTION=meth_hp1_avg=average methylation in haplotype 1 (higher mean methylation)");
		output.addComment("##DESCRIPTION=meth_hp1_std=standard deviation of methylation in haplotype 1 (higher mean methylation)");
		output.addComment("##DESCRIPTION=cov_hp1=average CpG coverage in haplotype 1 (higher mean methylation)");
		output.addComment("##DESCRIPTION=meth_hp2_avg=average methylation in haplotype 2 (lower mean methylation)");
		output.addComment("##DESCRIPTION=meth_hp2_std=standard deviation of methylation in haplotype 2 (lower mean methylation)");
		output.addComment("##DESCRIPTION=cov_hp2=average CpG coverage in haplotype 2 (lower mean methylation)");
		output.addComment("##DESCRIPTION=meth_nohp_avg=average methylation in non-haplotyped");
		output.addComment("##DESCRIPTION=meth_nohp_std=standard deviation of methylation in non-haplotyped");
		output.addComment("##DESCRIPTION=cov_nohp=average CpG coverage in non-haplotyped");
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
		output.addHeader("meth_nohp_avg");
		output.addHeader("meth_nohp_std");
		output.addHeader("cov_nohp");
		output.addHeader("meth_all_avg");
		output.addHeader("meth_all_std");
		output.addHeader("cov_all");


		QList<TabixIndexedFile> cohort_bed_files_hp1;
		QList<TabixIndexedFile> cohort_bed_files_hp2;
		QList<TabixIndexedFile> cohort_bed_files_all;
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
			cohort_bed_files_hp1.fill(TabixIndexedFile(), cohort_file_paths_hp1.size());
			cohort_bed_files_hp2.fill(TabixIndexedFile(), cohort_file_paths_hp2.size());
			cohort_bed_files_all.fill(TabixIndexedFile(), cohort_file_paths_all.size());
			//iterate over cohort
			for (int s = 0; s < cohort_file_paths_all.size(); ++s)
			{
				cohort_bed_files_hp1[s].load(cohort_file_paths_hp1[s].toUtf8());
				cohort_bed_files_hp2[s].load(cohort_file_paths_hp2[s].toUtf8());
				cohort_bed_files_all[s].load(cohort_file_paths_all[s].toUtf8());
			}
		}



		//open tabix indices

		//iterate over table
		for (int i = 0; i < imprinting_table.count(); ++i)
		{
			//get methylation
			const QStringList& line = imprinting_table[i];
			Chromosome chr(line.at(idx_chr));
			int start = Helper::toInt(line.at(idx_highlight_start), "highlight start", line.join(" "));
			int end = Helper::toInt(line.at(idx_highlight_end), "highlight end", line.join(" "));

			// extract methylation
			QByteArrayList methylation_hp1 = bed_hp1.getMatchingLines(chr, start, end);
			QByteArrayList methylation_hp2 = bed_hp2.getMatchingLines(chr, start, end);
			QByteArrayList methylation_all = bed_all.getMatchingLines(chr, start, end);

			// calaculate sample statistics
			QMap<QByteArray,double> sample_stats = getMethylation(methylation_hp1, methylation_hp2, methylation_all);

			//TODO: fill filter column
			QByteArray filter;


			//TODO:( write to output folder)


			// add cohort statistics
			QMap<QByteArray, double> cohort_stats;
			if (cohort_annotation)
			{
				QList<double> hp1_means;
				QList<double> hp2_means;
				QList<double> all_means;
				//iterate over cohort
				for (int s = 0; s < cohort_file_paths_all.size(); ++s)
				{
					//extract methylation
					QByteArrayList methylation_hp1_cohort = cohort_bed_files_hp1[s].getMatchingLines(chr, start, end);
					QByteArrayList methylation_hp2_cohort = cohort_bed_files_hp2[s].getMatchingLines(chr, start, end);
					QByteArrayList methylation_all_cohort = cohort_bed_files_all[s].getMatchingLines(chr, start, end);
					// calaculate sample statistics
					QMap<QByteArray,double> cohort_sample_stats = getMethylation(methylation_hp1_cohort, methylation_hp2_cohort, methylation_all_cohort);

					//TODO: filter NAN
					//store mean values
					hp1_means << cohort_sample_stats["hp1_avg"];
					hp2_means << cohort_sample_stats["hp2_avg"];
					all_means << cohort_sample_stats["all_avg"];

					// TODO: ( write to output folder)



				}
				// perform statistics
				cohort_stats["mean_hp1"] = NAN;
				cohort_stats["stdev_hp1"] = NAN;
				cohort_stats["zscore_hp1"] = NAN;
				cohort_stats["cohort_size_hp1"] = 0.0;
				cohort_stats["mean_hp2"] = NAN;
				cohort_stats["stdev_hp2"] = NAN;
				cohort_stats["zscore_hp2"] = NAN;
				cohort_stats["cohort_size_hp2"] = 0.0;
				cohort_stats["mean_all"] = NAN;
				cohort_stats["stdev_all"] = NAN;
				cohort_stats["zscore_all"] = NAN;
				cohort_stats["cohort_size_all"] = 0.0;

				if (hp1_means.size() > 0)
				{
					cohort_stats["cohort_size_hp1"] = hp1_means.size();
					cohort_stats["mean_hp1"] = BasicStatistics::mean(hp1_means);
					cohort_stats["stdev_hp1"] = BasicStatistics::stdev(hp1_means, cohort_stats["mean_hp1"]);
					if (cohort_stats["stdev_hp1"] != 0.0)
					{
						cohort_stats["zscore_hp1"] = (sample_stats["hp1_avg"] - cohort_stats["mean_hp1"]) / cohort_stats["stdev_hp1"];
					}
				}

				if (hp2_means.size() > 0)
				{
					cohort_stats["cohort_size_hp2"] = hp2_means.size();
					cohort_stats["mean_hp2"] = BasicStatistics::mean(hp2_means);
					cohort_stats["stdev_hp2"] = BasicStatistics::stdev(hp2_means, cohort_stats["mean_hp2"]);
					if (cohort_stats["stdev_hp2"] != 0.0)
					{
						cohort_stats["zscore_hp2"] = (sample_stats["hp2_avg"] - cohort_stats["mean_hp2"]) / cohort_stats["stdev_hp2"];
					}
				}

				if (all_means.size() > 0)
				{
					cohort_stats["cohort_size_all"] = all_means.size();
					cohort_stats["mean_all"] = BasicStatistics::mean(all_means);
					cohort_stats["stdev_all"] = BasicStatistics::stdev(all_means, cohort_stats["mean_all"]);
					if (cohort_stats["stdev_all"] != 0.0)
					{
						cohort_stats["zscore_all"] = (sample_stats["all_avg"] - cohort_stats["mean_all"]) / cohort_stats["stdev_all"];
					}
				}

			}


			// add methylation columns
			output.addRow(assemble_imprinting_line(line, filter, sample_stats, cohort_stats));



		}






		//combine to finial table

		//write output


	}
private:
	QMap<QByteArray,double> getMethylation(const QByteArrayList& methylation_hp1, const QByteArrayList& methylation_hp2, const QByteArrayList& methylation_all)
	{
		//const bedmethyl indices:
		const int idx_chr = 0;
		const int idx_start = 1;
		const int idx_end = 2;
		const int idx_mod_type = 3;
		const int idx_cov = 9;
		const int idx_frac = 10;
		const int idx_n_mod = 11;
		QMap<QByteArray, QPair<int, int>> methylation_unphased;
		QMap<QByteArray,double> return_values;

		//combined
		QList<double> coverage_all;
		QList<double> frac_all;
		foreach (const QByteArray& line, methylation_all)
		{
			QByteArrayList parts = line.split('\t');

			if (parts.at(idx_mod_type).trimmed() != "m") continue;

			int cov = Helper::toInt(parts.at(idx_cov), "N_cov_valid", line);
			int n_mod = Helper::toInt(parts.at(idx_n_mod), "N_mod", line);
			double frac = Helper::toDouble(parts.at(idx_frac), "fraction modified", line);
			coverage_all << cov;
			frac_all << frac;
			methylation_unphased.insert(parts.at(idx_chr) + ":" + parts.at(idx_start) + "-" + parts.at(idx_end), QPair<int, int>(n_mod, cov));
		}
		return_values.insert("all_avg", BasicStatistics::mean(frac_all));
		return_values.insert("all_std",  BasicStatistics::stdev(frac_all));
		return_values.insert("all_cov", BasicStatistics::mean(coverage_all));

		//hp1
		QList<double> coverage_hp1;
		QList<double> frac_hp1;
		foreach (const QByteArray& line, methylation_hp1)
		{
			QByteArrayList parts = line.split('\t');

			if (parts.at(idx_mod_type).trimmed() != "m") continue;

			int cov = Helper::toInt(parts.at(idx_cov), "N_cov_valid", line);
			int n_mod = Helper::toInt(parts.at(idx_n_mod), "N_mod", line);
			double frac = Helper::toDouble(parts.at(idx_frac), "fraction modified", line);
			coverage_hp1 << cov;
			frac_hp1 << frac;
			QByteArray cpg_site = parts.at(idx_chr) + ":" + parts.at(idx_start) + "-" + parts.at(idx_end);
			if (methylation_unphased.contains(cpg_site))
			{
				methylation_unphased[cpg_site].first -= n_mod;
				methylation_unphased[cpg_site].second -= cov;
			}

		}

		return_values.insert("hp1_avg", BasicStatistics::mean(frac_hp1));
		return_values.insert("hp1_std", BasicStatistics::stdev(frac_hp1));
		return_values.insert("hp1_cov", BasicStatistics::mean(coverage_hp1));

		//hp2
		QList<double> coverage_hp2;
		QList<double> frac_hp2;
		foreach (const QByteArray& line, methylation_hp2)
		{
			QByteArrayList parts = line.split('\t');

			if (parts.at(idx_mod_type).trimmed() != "m") continue;

			int cov = Helper::toInt(parts.at(idx_cov), "N_cov_valid", line);
			int n_mod = Helper::toInt(parts.at(idx_n_mod), "N_mod", line);
			double frac = Helper::toDouble(parts.at(idx_frac), "fraction modified", line);
			coverage_hp2 << cov;
			frac_hp2 << frac;
			QByteArray cpg_site = parts.at(idx_chr) + ":" + parts.at(idx_start) + "-" + parts.at(idx_end);
			if (methylation_unphased.contains(cpg_site))
			{
				methylation_unphased[cpg_site].first -= n_mod;
				methylation_unphased[cpg_site].second -= cov;
			}

		}

		return_values.insert("hp2_avg", BasicStatistics::mean(frac_hp2));
		return_values.insert("hp2_std", BasicStatistics::stdev(frac_hp2));
		return_values.insert("hp2_cov", BasicStatistics::mean(coverage_hp2));

		//no hp
		QList<double> coverage_nohp;
		QList<double> frac_nohp;
		foreach (auto pair, methylation_unphased)
		{
			coverage_nohp << pair.second;
			frac_nohp << ((double) pair.first / pair.second);
		}
		return_values.insert("nohp_avg", BasicStatistics::mean(frac_nohp));
		return_values.insert("nohp_std", BasicStatistics::stdev(frac_nohp));
		return_values.insert("nohp_cov", BasicStatistics::mean(coverage_nohp));


		return return_values;
	}


	QStringList assemble_imprinting_line(const QStringList& base_line, const QByteArray& filter_column, const QMap<QByteArray, double>& sample_stats, const QMap<QByteArray, double>& cohort_stats)
	{
		QStringList line = base_line;
		line << filter_column;

		//hp1
		line << QByteArray::number(sample_stats["hp1_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["hp1_std"], 'f', 3);
		line << QByteArray::number(sample_stats["hp1_cov"], 'f', 3);

		//hp2
		line << QByteArray::number(sample_stats["hp2_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["hp2_std"], 'f', 3);
		line << QByteArray::number(sample_stats["hp2_cov"], 'f', 3);

		//nohp
		line << QByteArray::number(sample_stats["nohp_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["nohp_std"], 'f', 3);
		line << QByteArray::number(sample_stats["nohp_cov"], 'f', 3);

		//all
		line << QByteArray::number(sample_stats["all_avg"], 'f', 3);
		line << QByteArray::number(sample_stats["all_std"], 'f', 3);
		line << QByteArray::number(sample_stats["all_cov"], 'f', 3);


		return line;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
