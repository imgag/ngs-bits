#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
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
		setDescription("Annotates a given TSV file with CNVs based on the gene information of the NGSD.");
		addInfile("in", "Input TSV file containing the CNVs.", false, true);
		addOutfile("out", "Output TSV file containing the annotated CNVs.", false, true);

		//optional
		addFlag("add_simple_gene_names", "Adds an additioal column containing only the list of gene names.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2019, 11, 11, "Initial version of this tool.");


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


		out << "annotate TSV file..." << endl;

		// copy comments
		TSVFileStream cnv_input_file(getInfile("in"));
		QByteArrayList output_buffer;
		output_buffer.append(cnv_input_file.comments());

		// modify header
		QByteArrayList header = cnv_input_file.header();
		if (add_simple_gene_names_) header.append("genes");
		header.append("gene_info");
		output_buffer << "#" + header.join("\t");

		// get indices for position
		int i_chr = cnv_input_file.colIndex("chr", true);
		int i_start = cnv_input_file.colIndex("start", true);
		int i_end = cnv_input_file.colIndex("end", true);

		// iterate over input file and annotate each cnv
		while (!cnv_input_file.atEnd())
		{
			// read next line:
			QByteArrayList tsv_line = cnv_input_file.readLine();

			// parse position:
			Chromosome chr = Chromosome(tsv_line[i_chr]);
			int start = Helper::toInt(tsv_line[i_start], "start");
			int end = Helper::toInt(tsv_line[i_end], "end");

			// get all matching BED entries
			QVector<int> matching_indices = gene_regions_index.matchingIndices(chr, start, end);

			// iterate over all matching BED entries and check coverage
			QByteArrayList gene_info;
			QByteArrayList gene_name_list;
			foreach (int index, matching_indices)
			{
				QByteArray gene_name = gene_regions[index].annotations()[0];
				QByteArray gnomad_oe_lof = db.geneInfo(gene_name).oe_lof.toUtf8();
				QByteArray covered_region;
				if (start <= gene_regions[index].start() && end >= gene_regions[index].end())
				{
					// CNV convers the whole gene
					covered_region = "complete";
				}
				else
				{
					if (!exon_regions.contains(gene_name))
					{
						// get exon/splicing region from database
						exon_regions[gene_name] = getGeneRegion(gene_name, db, "exon");
					}
					if (exon_regions[gene_name].overlapsWith(chr, start, end))
					{
						covered_region = "exonic/splicing";
					}
					else
					{
						covered_region = "intronic/intergenic";
					}
				}

				// add gene string
				gene_info.append(gene_name + " (oe_lof=" + gnomad_oe_lof + " region=" + covered_region + ")");
				// add gene name
				gene_name_list.append(gene_name);
			}

			// extend annotation
			QByteArray annotated_line = tsv_line.join("\t");
			if (add_simple_gene_names_) annotated_line += "\t" + gene_name_list.join(",");
			annotated_line += "\t" + gene_info.join(",");

			//add annotated line to buffer
			output_buffer << annotated_line;
		}

		out << "Writing output file..." << endl;
		// open output file and write annotated CNVs to file
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
	 *	returns a BedLine containing the whole extended gene region for the given gene
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


};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
