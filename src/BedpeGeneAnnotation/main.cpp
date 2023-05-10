#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "BedpeFile.h"
#include "NGSD.h"
#include "TSVFileStream.h"
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
		setDescription("Annotates a BEDPE file with gene information from the NGSD.");
		addInfile("in", "Input BEDPE file containing the SVs.", false, true);
		addOutfile("out", "Output BEDPE file containing the annotated SVs.", false, true);

		//optional
		addFlag("add_simple_gene_names", "Adds an additional column containing only the list of gene names.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2023, 5, 10, "Added column with gene names at breakpoints.");
		changeLog(2020, 1, 27, "Bugfix: 0-based BEDPE positions are now converted into 1-based BED positions.");
		changeLog(2020, 1, 21, "Added ability to reannotate BEDPE files by overwriting old annotation.");
		changeLog(2020, 1, 20, "Updated overlap method, refactored code.");
		changeLog(2020, 1, 14, "Added handling of duplicates.");
		changeLog(2020, 1, 8, "Initial version of this tool.");
	}

	virtual void main()
	{
		//init
		bool use_test_db_ = getFlag("test");
		bool add_simple_gene_names_ = getFlag("add_simple_gene_names");
		NGSD db(use_test_db_);
		QTextStream out(stdout);

		// start timer
		QTime timer;
		timer.start();

		//generate BED files for whole gene loci
		BedFile gene_regions;
		foreach (const QByteArray& gene_name, db.approvedGeneNames())
		{
			BedFile regions = db.geneToRegions(gene_name, Transcript::ENSEMBL, "gene", true, false);
			regions.extend(5000);
			gene_regions.add(regions);
		}
		gene_regions.sort();
		ChromosomalIndex<BedFile> gene_regions_index(gene_regions);
		out << "caching gene start/end finished (runtime: " << Helper::elapsedTime(timer) << ")" << endl;
		timer.restart();

		//cache gnomAD o/e LOF values
		QHash<QByteArray, QByteArray> gene_oe_lof;
		foreach (const QByteArray& gene_name, db.approvedGeneNames())
		{
			QVariant tmp = db.getValue("SELECT gnomad_oe_lof FROM geneinfo_germline WHERE symbol='" + gene_name + "'");
			if (tmp.isValid() && !tmp.isNull())
			{
				gene_oe_lof[gene_name] = QByteArray::number(tmp.toDouble(), 'f', 2);
			}
			else
			{
				gene_oe_lof[gene_name] = "n/a";
			}
		}
		out << "caching gnomAD o/e finished (runtime: " << Helper::elapsedTime(timer) << ")" << endl;
		timer.restart();

		// open input file
		BedpeFile bedpe_input_file;
		bedpe_input_file.load(getInfile("in"));

		// modify header if gene columns not already present
		QByteArrayList header = bedpe_input_file.annotationHeaders();
		int i_gene_column = bedpe_input_file.annotationIndexByName("GENES", false);
		if (add_simple_gene_names_ && i_gene_column < 0) header.append("GENES");
		int i_gene_info_column = bedpe_input_file.annotationIndexByName("GENE_INFO", false);
		if (i_gene_info_column < 0) header.append("GENE_INFO");

		// copy header to output
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_input_file.headers());
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t");

		// iterate over all structural variants
		QHash<QByteArray, BedFile> exon_regions;
		for (int i=0; i < bedpe_input_file.count(); i++)
		{
			BedpeLine line = bedpe_input_file[i];

			// get region of SV
			BedFile sv_region = line.affectedRegion();

			GeneSet matching_genes;
			QHash<QByteArray, QByteArray> covered_regions;

			// iterate over all BED entries
			for (int j = 0; j < sv_region.count(); ++j)
			{
				const BedLine& sv_entry = sv_region[j];

				// get all matching gene entries
				QVector<int> matching_indices = gene_regions_index.matchingIndices(sv_entry.chr(), sv_entry.start(), sv_entry.end());

				// iterate over all matching BED entries and check coverage
				foreach (int index, matching_indices)
				{
					// store gene name
					QByteArray gene_name = gene_regions[index].annotations()[0];
					matching_genes.insert(gene_name);

					// determine overlap of SV and gene
					QByteArray overlap;
					if (sv_entry.start() <= gene_regions[index].start() && sv_entry.end() >= gene_regions[index].end())
					{
						overlap = "complete";
					}
					else
					{
						if (!exon_regions.contains(gene_name))
						{
							BedFile regions = db.geneToRegions(gene_name, Transcript::ENSEMBL, "exon", true, false);
							regions.extend(20);
							exon_regions[gene_name] = regions;
						}
						if (exon_regions[gene_name].overlapsWith(sv_entry.chr(), sv_entry.start(), sv_entry.end()))
						{
							overlap = "exonic/splicing";
						}
						else
						{
							overlap = "intronic/intergenic";
						}
					}

					//determine maximum overlap
					if (covered_regions.contains(gene_name))
					{
						QByteArray overlap_old = covered_regions[gene_name];
						if (overlap != overlap_old)
						{
							if (overlap_old == "complete" || overlap == "complete")
							{
								covered_regions[gene_name] = "complete";
							}
							else if (overlap_old == "exonic/splicing" || overlap == "exonic/splicing")
							{
								covered_regions[gene_name] = "exonic/splicing";
							}
							// else: both intronic/intergenic
						}
					}
					else
					{
						covered_regions[gene_name] = overlap;
					}
				}
			}

			// join gene info
			QByteArrayList gene_info_entry;
			QByteArrayList gene_entry;
			foreach (const QByteArray& gene, matching_genes)
			{
				gene_entry.append(gene);
				gene_info_entry.append(gene + " (oe_lof=" + gene_oe_lof[gene] + " region=" + covered_regions[gene] + ")");
			}

			// generate annotated line
			QByteArrayList additional_annotations;

			// gene names
			if (add_simple_gene_names_)
			{
				if (i_gene_column >= 0)
				{
					QList<QByteArray> annotations = line.annotations();
					annotations[i_gene_column] = gene_entry.join(",");
					line.setAnnotations(annotations);
				}
				else
				{
					additional_annotations.append(gene_entry.join(","));
				}
			}

			// gene info
			if (i_gene_info_column >= 0)
			{
				QList<QByteArray> annotations = line.annotations();
				annotations[i_gene_info_column] = gene_info_entry.join(",");
				line.setAnnotations(annotations);
			}
			else
			{
				additional_annotations.append(gene_info_entry.join(","));
			}

			// extend annotation
			QByteArray annotated_line = line.toTsv();
			if (additional_annotations.size() > 0) annotated_line += "\t" + additional_annotations.join("\t");

			//add annotated line to buffer
			output_buffer << annotated_line;
		}

		// open output file and write annotated SVs to file
		QSharedPointer<QFile> sv_output_file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream output_stream(sv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		sv_output_file->close();


		out << "annotation complete (runtime: " << Helper::elapsedTime(timer) << ")." << endl;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
