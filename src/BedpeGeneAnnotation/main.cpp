#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "BedpeFile.h"
#include "NGSD.h"
#include "TSVFileStream.h"
#include <QElapsedTimer>
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
		setDescription("Annotates a given BEDPE file with SVs based on the gene information of the NGSD.");
		addInfile("in", "Input BEDPE file containing the SVs.", false, true);
		addOutfile("out", "Output BEDPE file containing the annotated SVs.", false, true);

		//optional
		addFlag("add_simple_gene_names", "Adds an additioal column containing only the list of gene names.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020, 1, 27, "Bugfix: 0-based BEDPE positions are now converted into 1-based BED positions.");
		changeLog(2020, 1, 21, "Added ability to reannotate BEDPE files by overwriting old annotation.");
		changeLog(2020, 1, 20, "Updated overlap method, refactored code.");
		changeLog(2020, 1, 14, "Added handling of duplicates.");
		changeLog(2020, 1, 8, "Initial version of this tool.");
	}

	virtual void main()
	{
		//init
		use_test_db_ = getFlag("test");
		add_simple_gene_names_ = getFlag("add_simple_gene_names");
		NGSD db(use_test_db_);
		QTextStream out(stdout);

		// start timer
		QElapsedTimer timer;
		timer.start();

		// generate BED files for whole gene region and exons:
		BedFile gene_regions;
		QHash<QByteArray, BedFile> exon_regions;

		// get gene names:
		GeneSet gene_names = db.approvedGeneNames();

		out << "parsing " << gene_names.count() << " genes ..." << endl;
		int parsed_genes = 0;
		// iterate over all gene names
		foreach (QByteArray gene_name, gene_names)
		{
			// generate bed file for complete genes
			gene_regions.add(getGeneRegion(gene_name, db, "gene"));

			// progress output
			parsed_genes++;
			if (parsed_genes % 10000 == 0)
			{
				out << "\t\t" << parsed_genes << " of " << gene_names.count() << " genes parsed." << endl;
			}
		}

		out << "sorting BED file..." << endl;
		gene_regions.sort();

		out << "generating BED index..." << endl;
		ChromosomalIndex<BedFile> gene_regions_index(gene_regions);

		out << "preprocessing finished (runtime: " << getTimeString(timer.elapsed()) << ")" << endl;


		out << "annotate BEDPE file..." << endl;
		// open input file
		BedpeFile bedpe_input_file;
		bedpe_input_file.load(getInfile("in"));

		// check if BEDPE file already contains gene annotation:
		int i_gene_column = bedpe_input_file.annotationIndexByName("GENES", false);
		int i_gene_info_column = bedpe_input_file.annotationIndexByName("GENE_INFO", false);

		// copy comments
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_input_file.comments());

		// get header
		QByteArrayList header = bedpe_input_file.annotationHeaders();

		// modify header if gene columns not already present
		if (add_simple_gene_names_ && i_gene_column < 0) header.append("GENES");
		if (i_gene_info_column < 0) header.append("GENE_INFO");

		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t");

		// iterate over all structural variants
		for (int i=0; i < bedpe_input_file.count(); i++)
		{
			BedpeLine line = bedpe_input_file[i];

			// get region of SV
			BedFile sv_region = getSvRegion(line);

			GeneSet matching_genes;
			QHash<QByteArray, QByteArray> gnomad_oe_lof_values;
			QHash<QByteArray, QByteArray> covered_regions;

			// iterate over all BED entries
			for (int j = 0; j < sv_region.count(); ++j)
			{
				BedLine sv_entry = sv_region[j];

				// get all matching gene entries
				QVector<int> matching_indices = gene_regions_index.matchingIndices(sv_entry.chr(), sv_entry.start(), sv_entry.end());

				// iterate over all matching BED entries and check coverage
				foreach (int index, matching_indices)
				{
					// store gene name
					QByteArray gene_name = gene_regions[index].annotations()[0].trimmed().toUpper();
					matching_genes.insert(gene_name);

					// store gnomad oe lof score
					gnomad_oe_lof_values[gene_name] = db.geneInfo(gene_name).oe_lof.toUtf8();

					// determine covered gene region
					QByteArray covered_region;
					if (sv_entry.start() <= gene_regions[index].start() && sv_entry.end() >= gene_regions[index].end())
					{
						// SV convers the whole gene
						covered_region = "complete";
					}
					else
					{
						if (!exon_regions.contains(gene_name))
						{
							// get exon/splicing region from database
							exon_regions[gene_name] = getGeneRegion(gene_name, db, "exon");
						}
						if (exon_regions[gene_name].overlapsWith(sv_entry.chr(), sv_entry.start(), sv_entry.end()))
						{
							covered_region = "exonic/splicing";
						}
						else
						{
							covered_region = "intronic/intergenic";
						}
					}

					if (covered_regions.contains(gene_name))
					{
						// merge regions
						if (covered_region != covered_regions[gene_name])
						{
							if ((covered_regions[gene_name] == "complete") || (covered_region == "complete"))
							{
								covered_regions[gene_name] = "complete";
							}
							else if ((covered_regions[gene_name] == "exonic/splicing") || (covered_region == "exonic/splicing"))
							{
								covered_regions[gene_name] = "exonic/splicing";
							}
							// else: both intronic/intergenic
						}
					}
					else
					{
						covered_regions[gene_name] = covered_region;
					}
				}
			}

			// join gene info
			QByteArrayList gene_info_entry;
			QByteArrayList gene_entry;
			foreach (QString gene, matching_genes)
			{
				gene_entry.append(gene.toUtf8());
				gene_info_entry.append(gene.toUtf8() + " (oe_lof=" + gnomad_oe_lof_values[gene.toUtf8()]
						+ " region=" + covered_regions[gene.toUtf8()] + ")");
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

		out << "Writing output file..." << endl;
		// open output file and write annotated SVs to file
		QSharedPointer<QFile> cnv_output_file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream output_stream(cnv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		cnv_output_file->close();


		out << "annotation complete (runtime: " << getTimeString(timer.elapsed()) << ")." << endl;

	}
private:
	bool use_test_db_;
	bool add_simple_gene_names_;

	/*
	 *  returns a formatted time string (QByteArray) from a given time in milliseconds
	 */
	QByteArray getTimeString(qint64 milliseconds)
	{
		QTime time(0,0,0);
		time = time.addMSecs(milliseconds);
		return time.toString("hh:mm:ss.zzz").toUtf8();
	}

	/*
	 *	returns a BED file containing the whole extended gene region for the given gene
	 */
	BedFile getGeneRegion(const QByteArray& gene_name, NGSD& db, const QByteArray& mode)
	{

		// calculate region
		GeneSet single_gene;
		single_gene.insert(gene_name);
		BedFile gene_regions = db.genesToRegions(single_gene, Transcript::ENSEMBL, mode, true, false);

		if (mode == "gene")
		{
			// extend and merge gene regions
			gene_regions.extend(5000);
			gene_regions.merge();
		}
		else
		{
			// extend by splice region
			gene_regions.extend(20);
		}


		for (int i = 0; i < gene_regions.count(); ++i)
		{
			// add gene name annotation
			QByteArrayList annotation;
			annotation.append(gene_name);
			gene_regions[i].annotations() = annotation;
		}
		return gene_regions;
	}

	/*
	 *  returns BED file containing the affected chromosomal region of a given SV
	 */
	BedFile getSvRegion(const BedpeLine& sv)
	{
		BedFile sv_region;

		// determine region based on SV type
		switch (sv.type())
		{
			case StructuralVariantType::INV:
			case StructuralVariantType::DEL:
			case StructuralVariantType::DUP:
				// whole area (+1 because BEDPE is 0-based)
				sv_region.append(BedLine(sv.chr1(), sv.start1() + 1, sv.end2() + 1));
				break;

			case StructuralVariantType::BND:
			case StructuralVariantType::INS:
				// consider pos 1 and pos 2 seperately (+1 because BEDPE is 0-based)
				sv_region.append(BedLine(sv.chr1(), sv.start1() + 1, sv.end1() + 1));
				sv_region.append(BedLine(sv.chr2(), sv.start2() + 1, sv.end2() + 1));
				break;

			default:
				break;
		}
		return sv_region;
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
