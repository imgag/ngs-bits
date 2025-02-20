#include "ToolBase.h"
#include "TabixIndexedFile.h"
#include "Helper.h"
#include "VariantList.h"

#include <QFile>
#include <QTextStream>


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
		setDescription("Calculates the Polgenic Risk Score for a given set of PRS VCFs");
		addInfile("in", "Tabix indexed VCF.GZ file of the sample.", false);
		addInfileList("prs", "List of PRS VCFs.", false);
		addOutfile("out", "Output TSV file", false);

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
		QByteArrayList column_headers = QByteArrayList() << "pgs_id" << "trait" << "score" << "normal_score" << "build" << "n_var" << "pgp_id" << "citation";
		output_tsv->write("#" + column_headers.join("\t") + "\n");

		//iterate over all given PRS files
		foreach (const QString& prs_file_path, getInfileList("prs"))
		{
			//load PRS file
			VariantList prs_variant_list;
			prs_variant_list.load(prs_file_path);

			//get indices
			int weight_idx = prs_variant_list.annotationIndexByName("WEIGHT");

			//define PRS
			double prs = 0;
			QHash<QByteArray,QByteArray> column_entries;

			//parse comment lines
			foreach(const QString& comment_line, prs_variant_list.comments())
			{
				foreach(const QByteArray& column_name, column_headers)
				{
					if(comment_line.startsWith("##" + column_name))
					{
						if(column_entries.contains(column_name))
						{
							THROW(FileParseException, "Comment section of PRS VCF '" + prs_file_path + "' contains more than one entry for '" +column_name + "'!");
						}
						column_entries[column_name] = comment_line.section('=', 1).trimmed().toUtf8();
					}
				}
			}

			//check if all required comment lines are present
			QByteArrayList col_entries_not_in_header = QByteArrayList() << "score" << "normal_score";
			foreach (const QByteArray& key, column_headers)
			{
				if(col_entries_not_in_header.contains(key)) continue;
				if(!column_entries.contains(key))
				{
					THROW(FileParseException, "Comment section of PRS VCF '" + prs_file_path + "' misses the entry for '" + key + "'!");
				}
			}

                        //iterate over all variants in PRS
			for(int i = 0; i < prs_variant_list.count(); ++i)
			{
				const Variant& prs_variant = prs_variant_list[i];
                                //get all matching variants at this position
				QByteArrayList matching_lines = sample_vcf.getMatchingLines(prs_variant.chr(), prs_variant.start(), prs_variant.end(), true);
				QByteArrayList matching_variants;
				foreach(const QByteArray& line, matching_lines)
				{
					// check if variant has same ref/alternative base(s)
					if((Sequence(line.split('\t')[3]) == prs_variant.ref()) && (Sequence(line.split('\t')[4]) == prs_variant.obs())) matching_variants.append(line);
				}

				if(matching_variants.size() > 1)
				{
					THROW(FileParseException, "Variant '" + prs_variant.toString() + "' occures multiple times in sample VCF!");
				}

				if(matching_variants.size() == 1)
				{
					//get genotype
					QByteArrayList split_line = matching_variants[0].split('\t');
					QByteArrayList format_header_items = split_line[8].split(':');
					QByteArrayList format_value_items = split_line[9].split(':');
					int genotype_idx = format_header_items.indexOf("GT");
					if(genotype_idx < 0) THROW(FileParseException, "Genotype information is missing for variant '" + prs_variant.toString() + "'!");
					QByteArray genotype = format_value_items[genotype_idx].trimmed();
					int allele_count = 0;
					if(genotype == "0/1") allele_count = 1;
					else if(genotype == "1/1") allele_count = 2;
					else THROW(FileParseException, "Invalid genotype '" + genotype + "' in variant " + prs_variant.toString() + "'!");

					//calculate PRS part
					double weight = Helper::toDouble(prs_variant.annotations()[weight_idx], "PRS weight");
					prs += weight * allele_count;

				}
			}

			QByteArrayList prs_line = QByteArrayList() << column_entries["pgs_id"] << column_entries["trait"] << QByteArray::number(prs) << "."
														<< column_entries["build"] << column_entries["n_var"] << column_entries["pgp_id"] << column_entries["citation"];
			output_tsv->write(prs_line.join("\t") + "\n");


			//print final PRS
            out << column_entries["pgs_id"] << ":\t" << prs << QT_ENDL;
		}






	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
