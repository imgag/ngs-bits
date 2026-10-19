#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "TsvFile.h"
#include "TabixIndexedFile.h"
#include "BasicStatistics.h"

#include <NGSHelper.h>
#include <QFileInfo>

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
		setDescription("Extracts methylations from a modkit bedmethyl file for a given set of imprinting sites for plotting.");
		addInfile("in_hp1", "Input modkit bedmethyl file for first allele (in BED.GZ format).", false);
		addInfile("in_hp2", "Input modkit bedmethyl file for second allele (in BED.GZ format).", false);
		addInfile("in_all", "Input modkit bedmethyl file for combined allele track (in BED.GZ format).", false);
		addInfile("sites", "Input table containing imprinting sites (in TSV format).", false);
		addOutfile("out", "Output plot data table in BED format.", false);
		addEnum("gender", "Gender of index sample.", false, QStringList() << "n/a" << "male" << "female");

		//optional
		addInfileList("cohort_hp1", "List of bedmethyl cohort files for first allele (in BED.GZ format).", true);
		addInfileList("cohort_hp2", "List of bedmethyl cohort files for second allele (in BED.GZ format).", true);
		addInfileList("cohort_all", "List of bedmethyl cohort files for combined allele track (in BED.GZ format).", true);
		addInfileList("cohort_phasing_tracks", "List of phasing track files for each cohort sample (in BED format).", true);


	}

	struct PlotDataLine
	{
		QByteArray identifier;

		//methylation index
		double hp1 = NAN;
		double hp2 = NAN;
		double all = NAN;

		//methylation cohort
		QList<double> cohort_hp1;
		QList<double> cohort_hp2;
		QList<double> cohort_all;

	};

	virtual void main()
	{
		// check inputs
		// - input cohort lists same lengths
		QStringList cohort_file_paths_hp1 = getInfileList("cohort_hp1");
		QStringList cohort_file_paths_hp2 = getInfileList("cohort_hp2");
		QStringList cohort_file_paths_all = getInfileList("cohort_all");
		QStringList cohort_file_paths_phasing_tracks = getInfileList("cohort_phasing_tracks");

		if ((cohort_file_paths_hp1.size() != cohort_file_paths_hp2.size())
			|| (cohort_file_paths_hp1.size() != cohort_file_paths_all.size())
			|| (cohort_file_paths_hp1.size() != cohort_file_paths_phasing_tracks.size()))
		{
			THROW(ArgumentException, "List of cohort BED files have to have the same length for each allele!");
		}
		const bool cohort_annotation = (cohort_file_paths_hp1.size() > 0);

		const QString gender = getEnum("gender");


		//parse table
		TsvFile imprinting_sites;
		imprinting_sites.load(getInfile("sites"));
		const int idx_name = imprinting_sites.columnIndex("identifier");
		const int idx_chr = imprinting_sites.columnIndex("gene chr");
		const int idx_highlight_start = imprinting_sites.columnIndex("highlight start");
		const int idx_highlight_end = imprinting_sites.columnIndex("highlight end");
		const int idx_gene_start = imprinting_sites.columnIndex("gene start");
		const int idx_gene_end = imprinting_sites.columnIndex("gene end");

		//parse input files
		QByteArrayList prefixes = QByteArrayList() << "hp1" << "hp2" << "all";
		QMap<QByteArray, TabixIndexedFile> bed_files;
		bed_files["hp1"].load(getInfile("in_hp1").toUtf8());
		bed_files["hp2"].load(getInfile("in_hp2").toUtf8());
		bed_files["all"].load(getInfile("in_all").toUtf8());

		//bedmethyl indices
		//const int bedmethyl_idx_chr = 0;
		const int bedmethyl_idx_start = 1;
		const int bedmethyl_idx_end = 2;
		const int bedmethyl_idx_mod_type = 3;
		const int bedmethyl_idx_frac = 10;

		//init output file
		QString out_path = getOutfile("out");
		BedFile output;

		//add header
		QByteArrayList header;
		header << "chr" << "start" << "end" << "identifier";
		//hp1
		header << "index_hp1";
		for (const QString& file_path : std::as_const(cohort_file_paths_hp1))
		{
			QFileInfo file_info(file_path);
			header << file_info.baseName().toUtf8() + "_hp1";
		}
		//hp2
		header << "index_hp2";
		for (const QString& file_path : std::as_const(cohort_file_paths_hp2))
		{
			QFileInfo file_info(file_path);
			header << file_info.baseName().toUtf8() + "_hp2";
		}
		//all
		header << "index_all";
		for (const QString& file_path : std::as_const(cohort_file_paths_all))
		{
			QFileInfo file_info(file_path);
			header << file_info.baseName().toUtf8() + "_all";
		}
		output.appendHeader("#" + header.join("\t"));
		//output.appendHeader("identifier");

		// output.addComment("##DESCRIPTION=meth_hp1_avg=average methylation in haplotype 1 (higher mean methylation)");
		// output.addComment("##DESCRIPTION=meth_hp1_std=standard deviation of methylation in haplotype 1 (higher mean methylation)");
		// output.addComment("##DESCRIPTION=cov_hp1=average CpG coverage in haplotype 1 (higher mean methylation)");
		// output.addComment("##DESCRIPTION=meth_hp2_avg=average methylation in haplotype 2 (lower mean methylation)");
		// output.addComment("##DESCRIPTION=meth_hp2_std=standard deviation of methylation in haplotype 2 (lower mean methylation)");
		// output.addComment("##DESCRIPTION=cov_hp2=average CpG coverage in haplotype 2 (lower mean methylation)");
		// output.addComment("##DESCRIPTION=meth_all_avg=average methylation");
		// output.addComment("##DESCRIPTION=meth_all_std=standard deviation of methylation");
		// output.addComment("##DESCRIPTION=cov_all=average CpG coverage");
		// output.addHeader("identifier");
		// output.addHeader("title");
		// output.addHeader("gene symbol");
		// output.addHeader("gene chr");
		// output.addHeader("gene start");
		// output.addHeader("gene end");
		// output.addHeader("highlight start");
		// output.addHeader("highlight end");
		// output.addHeader("filter");
		// output.addHeader("meth_hp1_avg");
		// output.addHeader("meth_hp1_std");
		// output.addHeader("cov_hp1");
		// output.addHeader("meth_hp2_avg");
		// output.addHeader("meth_hp2_std");
		// output.addHeader("cov_hp2");
		// output.addHeader("meth_all_avg");
		// output.addHeader("meth_all_std");
		// output.addHeader("cov_all");


		QList<QMap<QByteArray, TabixIndexedFile>> cohort_bed_files;
		QList<BedFile> cohort_phasing_tracks;
		if (cohort_annotation)
		{
			// // add additional columns
			// output.addHeader("cohort_mean_hp1");
			// output.addHeader("cohort_stdev_hp1");
			// output.addHeader("zscore_hp1");
			// output.addHeader("cohort_mean_hp2");
			// output.addHeader("cohort_stdev_hp2");
			// output.addHeader("zscore_hp2");
			// output.addHeader("cohort_mean_all");
			// output.addHeader("cohort_stdev_all");
			// output.addHeader("zscore_all");
			// output.addHeader("cohort_size");

			// open tabix files for cohort samples
			cohort_bed_files.fill(QMap<QByteArray, TabixIndexedFile>(), cohort_file_paths_hp1.size());
			cohort_phasing_tracks.fill(BedFile(), cohort_file_paths_phasing_tracks.size());
			//iterate over cohort
			for (int s = 0; s < cohort_file_paths_all.size(); ++s)
			{
				cohort_bed_files[s]["hp1"].load(cohort_file_paths_hp1[s].toUtf8());
				cohort_bed_files[s]["hp2"].load(cohort_file_paths_hp2[s].toUtf8());
				cohort_bed_files[s]["all"].load(cohort_file_paths_all[s].toUtf8());
				//load cohort phasing track files
				cohort_phasing_tracks[s].load(cohort_file_paths_phasing_tracks[s]);
				if (!cohort_phasing_tracks[s].isMergedAndSorted())
				{
					cohort_phasing_tracks[s].sort();
					cohort_phasing_tracks[s].merge(false);
				}
			}
		}



		//open tabix indices

		//iterate over table
		QMap<BedLine, PlotDataLine> plot_data;
		for (int i = 0; i < imprinting_sites.count(); ++i)
		{
			//get methylation
			const QStringList& line = imprinting_sites[i];
			Chromosome chr(line.at(idx_chr));
			int highlight_start = Helper::toInt(line.at(idx_highlight_start), "highlight start", line.join(" "));
			int highlight_end = Helper::toInt(line.at(idx_highlight_end), "highlight end", line.join(" "));
			int gene_start = Helper::toInt(line.at(idx_gene_start), "gene start", line.join(" "));
			int gene_end = Helper::toInt(line.at(idx_gene_end), "gene end", line.join(" "));
			QByteArray identifier = line.at(idx_name).toUtf8();

			// extract methylation (index)
			double avg_hp1 = averageMethylation(bed_files["hp1"].getMatchingLines(chr, highlight_start, highlight_end));
			double avg_hp2 = averageMethylation(bed_files["hp2"].getMatchingLines(chr, highlight_start, highlight_end));
			bool switch_haplotypes = (!std::isnan(avg_hp2)) && (std::isnan(avg_hp1) || avg_hp1 < avg_hp2);

			for (const QByteArray& prefix : std::as_const(prefixes))
			{
				QByteArrayList lines = bed_files[prefix].getMatchingLines(chr, gene_start, gene_end);
				for (const QByteArray& line : std::as_const(lines))
				{
					QByteArrayList parts = line.split('\t');
					if (parts.size() < 13) THROW(FileParseException, "Invalid BedMethyl line: '" + line + "'!");
					if (parts.at(bedmethyl_idx_mod_type).trimmed() != "m") continue; // skip

					int start =  Helper::toInt(parts.at(bedmethyl_idx_start), "start", line) + 1; //+1 to convert to 1-based (internal)
					int end =  Helper::toInt(parts.at(bedmethyl_idx_end), "end", line);
					BedLine pos = BedLine(chr, start, end);
					double frac =  Helper::toDouble(parts.at(bedmethyl_idx_frac), "fraction modified", line);

					if(!plot_data.contains(pos))
					{
						PlotDataLine plot_line;
						plot_line.identifier = identifier;
						plot_line.cohort_hp1.fill(NAN, cohort_file_paths_hp1.size());
						plot_line.cohort_hp2.fill(NAN, cohort_file_paths_hp2.size());
						plot_line.cohort_all.fill(NAN, cohort_file_paths_all.size());
						plot_data.insert(pos, plot_line);
					}
					if (prefix == "hp1")
					{
						if (switch_haplotypes) plot_data[pos].hp2 = frac;
						else plot_data[pos].hp1 = frac;
					}
					else if (prefix == "hp2")
					{
						if (switch_haplotypes) plot_data[pos].hp1 = frac;
						else plot_data[pos].hp2 = frac;
					}
					else plot_data[pos].all = frac;
				}
			}

			// extract methylation (cohort)
			if (cohort_annotation)
			{
				//iterate over cohort
				for (int s = 0; s < cohort_file_paths_all.size(); ++s)
				{
					//skip cohort sample where the phasing is off:
					bool correctly_phased = (checkPhasing(BedLine(chr, highlight_start, highlight_end), cohort_phasing_tracks[s], gender).isEmpty());
					bool switch_haplotypes = false;
					if (correctly_phased)
					{
						double avg_hp1 = averageMethylation(cohort_bed_files[s]["hp1"].getMatchingLines(chr, highlight_start, highlight_end));
						double avg_hp2 = averageMethylation(cohort_bed_files[s]["hp2"].getMatchingLines(chr, highlight_start, highlight_end));
						switch_haplotypes = (!std::isnan(avg_hp2)) && (std::isnan(avg_hp1) || avg_hp1 < avg_hp2);
					}


					for (const QByteArray& prefix : std::as_const(prefixes))
					{
						if (!correctly_phased && (prefix != "all")) continue;
						QByteArrayList lines = cohort_bed_files[s][prefix].getMatchingLines(chr, gene_start, gene_end);
						for (const QByteArray& line : std::as_const(lines))
						{
							QByteArrayList parts = line.split('\t');
							if (parts.size() < 13) THROW(FileParseException, "Invalid BedMethyl line: '" + line + "'!");
							if (parts.at(bedmethyl_idx_mod_type).trimmed() != "m") continue; // skip

							int start =  Helper::toInt(parts.at(bedmethyl_idx_start), "start", line) + 1; //+1 to convert to 1-based (internal)
							int end =  Helper::toInt(parts.at(bedmethyl_idx_end), "end", line);
							BedLine pos = BedLine(chr, start, end);
							double frac =  Helper::toDouble(parts.at(bedmethyl_idx_frac), "fraction modified", line);

							if(!plot_data.contains(pos))
							{
								PlotDataLine plot_line;
								plot_line.identifier = identifier;
								plot_line.cohort_hp1.fill(NAN, cohort_file_paths_hp1.size());
								plot_line.cohort_hp2.fill(NAN, cohort_file_paths_hp2.size());
								plot_line.cohort_all.fill(NAN, cohort_file_paths_all.size());
								plot_data.insert(pos, plot_line);
							}
							if (prefix == "hp1")
							{
								if (switch_haplotypes) plot_data[pos].cohort_hp2[s] = frac;
								else plot_data[pos].cohort_hp1[s] = frac;
							}
							else if (prefix == "hp2")
							{
								if (switch_haplotypes) plot_data[pos].cohort_hp1[s] = frac;
								else plot_data[pos].cohort_hp2[s] = frac;
							}
							else plot_data[pos].cohort_all[s] = frac;
						}

					}
				}
			}
		}

		//write to file
		for (auto it = plot_data.cbegin(); it != plot_data.cend(); ++it)
		{
			BedLine bed_line = it.key();
			bed_line.annotations() = assembleBedAnnotation(it.value());
			output.append(bed_line);
		}
		output.store(out_path);


	}
private:
	double averageMethylation(const QByteArrayList& methylation_lines)
	{
		const int idx_mod_type = 3;
		const int idx_frac = 10;

		QList<double> frac_all;
		foreach (const QByteArray& line, methylation_lines)
		{
			QByteArrayList parts = line.split('\t');
			if (parts.size() < 13) THROW(FileParseException, "Invalid BedMethyl line: '" + line + "'!");
			if (parts.at(idx_mod_type).trimmed() != "m") continue;
			frac_all << Helper::toDouble(parts.at(idx_frac), "fraction modified", line);
		}

		return (frac_all.size() > 0)? BasicStatistics::mean(frac_all): NAN;
	}

	QByteArray checkPhasing(const BedLine& region, const BedFile& phasing_track, const QString& gender)
	{
		//check phasing track
		if (phasing_track.count() == 0) return "NoPhasingInfo";
		BedFile intersection;
		intersection.append(region);
		intersection.intersect(phasing_track);
		if (intersection.count() > 1) return "MultiplePhasingBlocks";
		if (intersection.count() < 1)
		{
			if (region.chr().isAutosome() || (region.chr().isX() && gender != "male")) return "UnphasedRegion";
			else if (region.chr().isGonosome() && NGSHelper::pseudoAutosomalRegion().overlapsWith(region)) return "UnphasedRegion";
			//else: single haplotype (e.g. chrM, chrY, ...)
			return "";
		}
		//exactly 1 phasing block --> check if it covers whole region
		if (!(intersection[0] == region)) return "PartlyUnphasedRegion";
		//whole region is phased
		return "";
	}


	QByteArrayList assembleBedAnnotation(const PlotDataLine& plot_data)
	{
		QByteArrayList annotation;
		annotation << plot_data.identifier;
		//hp1
		annotation << QByteArray::number(plot_data.hp1, 'f', 3);
		for (int i = 0; i < plot_data.cohort_hp1.size(); ++i)
		{
			annotation << QByteArray::number(plot_data.cohort_hp1.at(i), 'f', 3);
		}

		//hp2
		annotation << QByteArray::number(plot_data.hp2, 'f', 3);
		for (int i = 0; i < plot_data.cohort_hp2.size(); ++i)
		{
			annotation << QByteArray::number(plot_data.cohort_hp2.at(i), 'f', 3);
		}

		//all
		annotation << QByteArray::number(plot_data.all, 'f', 3);
		for (int i = 0; i < plot_data.cohort_all.size(); ++i)
		{
			annotation << QByteArray::number(plot_data.cohort_all.at(i), 'f', 3);
		}

		return annotation;
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
