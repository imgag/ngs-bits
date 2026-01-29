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
		BedFile gene_regions;
		for (const QByteArray& gene_name : db.approvedGeneNames())
		{
			QByteArrayList annos = QByteArrayList() << gene_name;
			int gene_id = db.geneId(gene_name);
			foreach(const Transcript& t, db.transcripts(gene_id, Transcript::ENSEMBL, false))
			{
				if (t.isPreferredTranscript() || t.isManeSelectTranscript() || t.isManePlusClinicalTranscript() || t.isGencodePrimaryTranscript())
				{
					gene_regions.append(BedLine(t.chr(), t.start(), t.end(), annos));
				}
			}
		}
		gene_regions.extend(5000);
		gene_regions.merge(true, true, true);
		ChromosomalIndex<BedFile> gene_regions_index(gene_regions);
		out << "caching gene start/end finished (runtime: " << Helper::elapsedTime(timer) << ")" << Qt::endl;
		timer.restart();

        out << "annotating CNV file..." << Qt::endl;

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
				const BedLine& gene_locus = gene_regions[index];

				QByteArray gene_name = gene_locus.annotations()[0];
				QByteArray gnomad_oe_lof = db.geneInfo(gene_name).oe_lof.toUtf8();

				//determine overlap of CNV and gene
				QByteArray covered_region;
				if (start <= (gene_locus.start()+5000) && end >= (gene_locus.end()-5000)) //correct by 5000 because gene loci are extended by 5000 for correct gene annotation
				{
					covered_region = "complete";
				}
				else
				{
					if (!exon_regions.contains(gene_name)) //get exon/splicing region of gene
					{
						BedFile regions;
						int gene_id = db.geneId(gene_name);
						foreach(const Transcript& t, db.transcripts(gene_id, Transcript::ENSEMBL, false))
						{
							if (t.isPreferredTranscript() || t.isManeSelectTranscript() || t.isManePlusClinicalTranscript() || t.isGencodePrimaryTranscript())
							{
								regions.add(t.regions());
							}
						}
						regions.extend(20);
						regions.merge();
						exon_regions[gene_name] = regions;
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

        out << "Writing output file..." << Qt::endl;
		// open output file and write annotated CNVs to file
		QSharedPointer<QFile> cnv_output_file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream output_stream(cnv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		cnv_output_file->close();

        out << "annotation done (runtime: " << Helper::elapsedTime(timer) << ")." << Qt::endl;
	}
private:
	bool use_test_db_;
	bool add_simple_gene_names_;
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
