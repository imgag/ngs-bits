#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "BedFile.h"
#include "NGSD.h"
#include "TSVFileStream.h"

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
		setDescription("Annotates TSV file containing CNVs with gene information from NGSD.");
		addInfile("in", "Input TSV file containing the CNVs.", false, true);
		addOutfile("out", "Output TSV file containing the annotated CNVs.", false, true);

		//optional
		addFlag("add_simple_gene_names", "Adds an additional column containing only the list of gene names.");
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

		// generate whole gene BED file and index
		GeneSet gene_names = db.approvedGeneNames();
        out << "Getting gene regions from NGSD for " << gene_names.count() << " genes ..." << QT_ENDL;
		BedFile gene_regions;
        for (const QByteArray& gene_name : gene_names)
		{
			gene_regions.add(getGeneRegion(gene_name, db, "gene"));
		}
		if (!gene_regions.isSorted()) gene_regions.sort();
		ChromosomalIndex<BedFile> gene_regions_index(gene_regions);
        out << "preprocessing done (runtime: " << Helper::elapsedTime(timer) << ")" << QT_ENDL;
		timer.restart();

        out << "annotating CNV file..." << QT_ENDL;

		// copy comments
		TSVFileStream cnv_input_file(getInfile("in"));
		QByteArrayList output_buffer;
		output_buffer.append(cnv_input_file.comments());

		//check if gene annotation already exists:
		QByteArrayList header = cnv_input_file.header();
		int i_genes = header.indexOf("genes");
		int i_gene_info = header.indexOf("gene_info");

		// modify header if neccessary
		if (i_genes<0 && add_simple_gene_names_) header.append("genes");
		if (i_gene_info < 0) header.append("gene_info");
		output_buffer << "#" + header.join("\t");

		// get indices for position
		int i_chr = cnv_input_file.colIndex("chr", true);
		int i_start = cnv_input_file.colIndex("start", true);
		int i_end = cnv_input_file.colIndex("end", true);

		// iterate over input file and annotate each cnv
		QHash<QByteArray, BedFile> exon_regions;
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
					// get exon/splicing region from database
					if (!exon_regions.contains(gene_name))
					{
						exon_regions[gene_name] = getGeneRegion(gene_name, db, "exon");
					}

					// annotate overlap type
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

			// update annotation
			if (add_simple_gene_names_)
			{
				if (i_genes < 0) tsv_line.append(gene_name_list.join(","));
				else tsv_line[i_genes] = gene_name_list.join(",");
			}
			if (i_gene_info < 0) tsv_line.append(gene_info.join(","));
			else tsv_line[i_gene_info] = gene_info.join(",");

			//add annotated line to buffer
			output_buffer << tsv_line.join("\t");
		}

        out << "Writing output file..." << QT_ENDL;
		// open output file and write annotated CNVs to file
		QSharedPointer<QFile> cnv_output_file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream output_stream(cnv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		cnv_output_file->close();

        out << "annotation done (runtime: " << Helper::elapsedTime(timer) << ")." << QT_ENDL;
	}
private:
	bool use_test_db_;
	bool add_simple_gene_names_;

	/*
	 *	returns a BedLine containing the whole extended gene region for the given gene
	 */
	BedFile getGeneRegion(const QByteArray& gene_name, NGSD& db, const QByteArray& mode)
	{
		// calculate region
		BedFile gene_regions = db.geneToRegions(gene_name, Transcript::ENSEMBL, mode, true, false);

		if (mode=="gene")
		{
			gene_regions.extend(5000);
			gene_regions.merge();
		}
		else
		{
			gene_regions.extend(20);
		}

		// add gene name annotation
		for (int i = 0; i < gene_regions.count(); ++i)
		{
			gene_regions[i].annotations() = QByteArrayList() << gene_name;
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
