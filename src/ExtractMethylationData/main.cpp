#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include "Settings.h"
#include <QFileInfo>
#include "TabixIndexedFile.h"
#include "FastaFileIndex.h"

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
		setDescription("Extracts the methylation state for a given set of loci");
		addInfile("in", "Tabix indexed BED.GZ file that contains the methylation info for each base (modkit).", false, true);
		addInfile("loci", "BED file containig position and strand of intrest", false, true);
		//optional
		addOutfile("out", "Output BED file containing combined methylation info of provided loci. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("add_methylation_types", "Also report 5mC (m) and 5hmC (h) entries as separate columns");
		addFlag("skip_invalid_sites", "Skip invalid CpG sites instead of aborting.");

		//changelog
		changeLog(2024,  6, 26, "Initial commit.");
		changeLog(2024,  7, 18, "Added option to add separate columns for 5mC/5hmC.");
		changeLog(2024,  9, 27, "Added option to ignore invalid CpG sites.");
	}

	virtual void main()
	{
		//init
		QString methylation_file_path = getInfile("in");
		QString loci_file_path = getInfile("loci").toUtf8();
		QString output_file_path = getOutfile("out").toUtf8();
		QString ref_file = getInfile("ref");
		bool add_methylation_types = getFlag("add_methylation_types");
		bool skip_invalid_sites = getFlag("skip_invalid_sites");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//load ref file
		FastaFileIndex ref_idx(ref_file);

		//load methyl file (index)
		TabixIndexedFile methylation_idx;
		methylation_idx.load(methylation_file_path.toUtf8());


		BedFile loci;
		loci.load(loci_file_path);

		BedFile output;
		QByteArrayList output_header = loci.headers().last().trimmed().split('\t');
		output_header.append("fraction_modified");
		output_header.append("N_valid_cov");
		output_header.append("N_mod");
		if (add_methylation_types)
		{
			output_header.append("fraction_modified_5mC");
			output_header.append("N_valid_cov_5mC");
			output_header.append("N_mod_5mC");
			output_header.append("fraction_modified_5hmC");
			output_header.append("N_valid_cov_5hmC");
			output_header.append("N_mod_5hmC");
		}
		output.appendHeader(output_header.join("\t"));

		for (int i = 0; i < loci.count(); ++i)
		{
			BedLine bed_line = loci[i];

			//validate length (CpG only):
			if (bed_line.length() != 2)
			{
				if (skip_invalid_sites)
				{
					qDebug() << "A CpG site has to be 2 bp long! " + bed_line.toString(true);
					continue;
				}
				else THROW(ArgumentException, "A CpG site has to be 2 bp long! " + bed_line.toString(true));
			}
			QByteArray strand = bed_line.annotations().at(0).trimmed();
			if (!((strand == "+") || (strand == "-"))) THROW(ArgumentException, "Strand has to be '+' or '-'! " + bed_line.toString(true));

			//get position of C/G
			int pos = ((strand == "+")?bed_line.start():bed_line.end());
			QByteArray mod_base = ((strand == "+")?"C":"G");
			//validate C position
			try
			{
				if (ref_idx.seq(bed_line.chr(), pos, 1, true) != mod_base)
				{
					if (skip_invalid_sites)
					{
						qDebug() << "Invalid " + mod_base + " position (is actually " + ref_idx.seq(bed_line.chr(), pos, 1, true) + " for CpG site)! " + bed_line.toString(true);
						continue;
					}
					else THROW(ArgumentException, "Invalid " + mod_base + " position (is actually " + ref_idx.seq(bed_line.chr(), pos, 1, true) + " for CpG site)! " + bed_line.toString(true));
				}

			}
			catch (Exception& e)
			{
				if (skip_invalid_sites)
				{
					QTextStream(stderr) << e.message();
					continue;
				}
				else
				{
					THROW(ArgumentException, e.message());
				}
			}

			//get entries from methylation file:
			QByteArrayList matches = methylation_idx.getMatchingLines(bed_line.chr(), pos, pos, false);

			float fraction_modified = 0.0f;
			int n_valid_cov = -1;
			int n_mod = 0;
			int entry_count = 0; //should be excatly 2 (h + m)

			//vars for seperate columns
			float fraction_modified_5mC = 0.0f;
			int n_valid_cov_5mC = 0;
			int n_mod_5mC = 0;
			float fraction_modified_5hmC = 0.0f;
			int n_valid_cov_5hmC = 0;
			int n_mod_5hmC = 0;


			foreach (const QByteArray& match, matches)
			{
				QByteArrayList parts = match.split('\t');
				if (parts.size() != 10) THROW(ArgumentException, "Invalid number of columns at '" + parts.join(" ") + "'! Should be 10 is " + QByteArray::number(parts.size()));

				//skip other strand
				if (strand != parts.at(5)) continue;
				//skip other mod bases:
				if (!((parts.at(3) == "h") || (parts.at(3) == "m"))) continue;

				QByteArrayList mod_parts = parts.at(9).split(' ');
				if (mod_parts.size() != 9) THROW(ArgumentException, "Invalid number of mod entries at '" + parts.join(" ") + "'! Should be 9 is " + QByteArray::number(mod_parts.size()));

				//validate N_valid_cov entry (should be the same for all entries)
				int cur_cov = Helper::toInt(mod_parts.at(0), "N_valid_cov", match);
				if (n_valid_cov == -1) n_valid_cov = cur_cov; //first occurence: -> apply depth
				else if (n_valid_cov != cur_cov) THROW(ArgumentException, "Mismatch in 'N_valid_cov' count at '" + match + "'!");//mismatch in N_valid_cov: -> should not happen

				//update modification fraction
				float cur_fraction_modified = Helper::toDouble(mod_parts.at(1), "fraction_modified", match);
				fraction_modified += cur_fraction_modified;
				int cur_n_mod = Helper::toInt(mod_parts.at(2), "N_mod", match);
				n_mod += cur_n_mod;

				//update individual columns
				if (add_methylation_types)
				{
					if (parts.at(3) == "m")
					{
						if (fraction_modified_5mC != 0.0f) THROW(ArgumentException, "Methylation for line '" + parts.join(" ") + "' occures twice in the file!");
						fraction_modified_5mC = cur_fraction_modified;
						n_valid_cov_5mC = cur_cov;
						n_mod_5mC = cur_n_mod;
					}
					else // mod base == "h"
					{
						if (fraction_modified_5hmC != 0.0f) THROW(ArgumentException, "Methylation for line '" + parts.join(" ") + "' occures twice in the file!");
						fraction_modified_5hmC = cur_fraction_modified;
						n_valid_cov_5hmC = cur_cov;
						n_mod_5hmC = cur_n_mod;
					}
				}

				entry_count++;
			}

			if (entry_count == 2)
			{
				// add info to output file
				bed_line.annotations().append(QByteArray::number(fraction_modified));
				bed_line.annotations().append(QByteArray::number(n_valid_cov));
				bed_line.annotations().append(QByteArray::number(n_mod));
			}
			else if (entry_count == 1)
			{
				qDebug() << "WARNING: Position " + bed_line.toString(true) + " only covered once in methylation file!";
				bed_line.annotations().append(QByteArray::number(fraction_modified));
				bed_line.annotations().append(QByteArray::number(n_valid_cov));
				bed_line.annotations().append(QByteArray::number(n_mod));
			}
			else if (entry_count == 0)
			{
				qDebug() << "WARNING: Position " + bed_line.toString(true) + " not covered in methylation file!";
				bed_line.annotations().append("");
				bed_line.annotations().append("");
				bed_line.annotations().append("");

			}
			else
			{
				THROW(ArgumentException, "Invalid entry count " + QByteArray::number(entry_count) + " (should be 2)! " + matches.join("\n"));
			}

			//append individual columns
			if (add_methylation_types)
			{
				bed_line.annotations().append(QByteArray::number(fraction_modified_5mC));
				bed_line.annotations().append(QByteArray::number(n_valid_cov_5mC));
				bed_line.annotations().append(QByteArray::number(n_mod_5mC));
				bed_line.annotations().append(QByteArray::number(fraction_modified_5hmC));
				bed_line.annotations().append(QByteArray::number(n_valid_cov_5hmC));
				bed_line.annotations().append(QByteArray::number(n_mod_5hmC));
			}
			output.append(bed_line);


		}

		//write output
		output.store(output_file_path);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

