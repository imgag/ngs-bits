#include "ToolBase.h"
#include "TabixIndexedFile.h"
#include "Helper.h"
#include "VcfFile.h"

#include <QFile>
#include <QTextStream>

//TODO: Handle missing variants (check in BAM for depth, check target region if region is contained > use AF if missing?)

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
		setDescription("Calculates the Polgenic Risk Score(s) for a sample.");
		setExtendedDescription(QStringList() << "The PRS VCF files have to contain a WEIGHT entry in the INFO column." << "Additionally some information about the PRS score is required in the VCF header." << "An example VCF file can be found at https://github.com/imgag/ngs-bits/blob/master/src/tools-TEST/data_in/VcfCalculatePRS_prs2.vcf");

		addInfile("in", "Tabix indexed VCF.GZ file of a sample.", false);
		addInfileList("prs", "List of PRS VCFs.", false);
		addOutfile("out", "Output TSV file containing Scores and PRS details", false);

		changeLog(2020,  7, 22, "Initial version of this tool.");
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);

		//load sample VCF
		TabixIndexedFile sample_vcf;
		sample_vcf.load(getInfile("in").toUtf8());

		//open output file and write header
		QSharedPointer<QFile> output_tsv = Helper::openFileForWriting(getOutfile("out"), true);
		QByteArrayList column_headers = QByteArrayList() << "pgs_id" << "trait" << "score" << "percentile" << "build" << "n_var" << "pgp_id" << "citation";
		output_tsv->write("#" + column_headers.join("\t") + "\n");

		//iterate over all given PRS files
		foreach (const QString& prs_file_path, getInfileList("prs"))
		{
			//load PRS file
			VcfFile prs_variant_list;
			prs_variant_list.load(prs_file_path);

			//does not support multi sample
			if(prs_variant_list.sampleIDs().count() > 1)
			{
				THROW(FileParseException, "PRS VCF file must not contain more than one sample: " + prs_file_path);
			}

			//define PRS
			double prs = 0;
			QVector<double> percentiles;
			QHash<QByteArray,QByteArray> column_entries;

			//parse comment lines
			foreach(const VcfHeaderLine& comment_line, prs_variant_list.vcfHeader().comments())
			{

				foreach(const QByteArray& column_name, column_headers)
				{
					if(comment_line.key.startsWith(column_name))
					{
						if(column_entries.contains(column_name))
						{
							THROW(FileParseException, "Comment section of PRS VCF  file contains more than one entry for '" +column_name + "': " + prs_file_path);
						}
						column_entries[column_name] = comment_line.value.trimmed();
						break;
					}
				}

				if(comment_line.key.startsWith("percentiles"))
				{
					if (percentiles.size() != 0) THROW(FileParseException, "Percentiles in PRS VCF file given twice: " + prs_file_path);
					QByteArrayList percentile_string = comment_line.value.trimmed().split(',');
					if (percentile_string.size() != 100) THROW(FileParseException, "Invalid number of percentiles given (required: 100, given: "  + QByteArray::number(percentile_string.size()) + ": " + prs_file_path);
					foreach (const QByteArray& value_string, percentile_string)
					{
						percentiles.append(Helper::toDouble(value_string, "Percentile"));
					}
				}
			}

			//check if all required comment lines are present
			QByteArrayList col_entries_not_in_header = QByteArrayList() << "score" << "percentile";
			foreach (const QByteArray& key, column_headers)
			{
				if(col_entries_not_in_header.contains(key)) continue;
				if(!column_entries.contains(key))
				{
					THROW(FileParseException, "Comment section of PRS VCFs does not contsin an entry for '" + key + "': " + prs_file_path);
				}
			}

			//iterate over all variants in PRS
			int c_found = 0;
			for(int i = 0; i < prs_variant_list.count(); ++i)
			{	
				const VcfLine& prs_variant = prs_variant_list[i];

				//does not support multi-allelic variants
				if(prs_variant.isMultiAllelic())
				{
					THROW(FileParseException, "Multi-allelic variants in PRS VCF files are not supported: " + prs_variant.variantToString());
				}

				//get all matching variants at this position
				QByteArrayList matching_lines = sample_vcf.getMatchingLines(prs_variant.chr(), prs_variant.start(), prs_variant.end(), true);
				QByteArrayList matching_variants;
				foreach(const QByteArray& line, matching_lines)
				{
					// check if overlapping variant is actually the one we are looking for
					QByteArrayList parts = line.split('\t');
					if(parts[1].toInt()==prs_variant.start() && parts[3]==prs_variant.ref() && parts[4]==prs_variant.alt(0))
					{
						matching_variants.append(line);
					}
				}

				if(matching_variants.size() > 1)
				{
					THROW(FileParseException, "Variant occures multiple times in sample VCF: " +  prs_variant.variantToString());
				}

				if(matching_variants.size() == 1)
				{
					//get genotype
					QByteArrayList split_line = matching_variants[0].split('\t');
					QByteArrayList format_header_items = split_line[8].split(':');
					QByteArrayList format_value_items = split_line[9].split(':');
					int genotype_idx = format_header_items.indexOf("GT");
					if(genotype_idx < 0) THROW(FileParseException, "Genotype information is missing for sample variant: " + matching_variants[0]);

					int allele_count = format_value_items[genotype_idx].count('1');

					if (allele_count > 2) THROW(FileParseException, "Invalid genotype '" + format_value_items[genotype_idx].trimmed() + "' in sample variant: " + matching_variants[0]);

					//calculate PRS part
					double weight = Helper::toDouble(prs_variant.info("WEIGHT"), "PRS weight");
					prs += weight * allele_count;

					++c_found;
				}
			}

			// compute percentile
			int percentile = -1;
			QByteArray percentile_string = ".";
			if (percentiles.size() == 100)
			{
				for (int i = 0; i < percentiles.size(); ++i)
				{
					if (prs < percentiles[i])
					{
						percentile = i + 1;
						break;
					}
				}
				percentile_string = (percentile == -1) ? "100" : QByteArray::number(percentile);
			}
			else if (percentiles.size() != 0)
			{
				THROW(FileParseException, "Invalid number of percentiles given (required: 100 or 0, given: "  + QByteArray::number(percentiles.size()) + ")!");
			}

			QByteArrayList prs_line = QByteArrayList() << column_entries["pgs_id"] << column_entries["trait"] << QByteArray::number(prs)
													   << percentile_string << column_entries["build"] << column_entries["n_var"]
													   << column_entries["pgp_id"] << column_entries["citation"];
			output_tsv->write(prs_line.join("\t") + "\n");


			//print final PRS
			out << column_entries["pgs_id"] << ": variants_found=" << c_found << " prs=" << prs << " percentile=" << percentile_string << endl;
		}

		output_tsv->flush();
		output_tsv->close();

	}
};


#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
