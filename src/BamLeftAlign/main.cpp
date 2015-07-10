#include <QTextStream>
#include "api/BamAlignment.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "LeftAlign.h"
#include "Settings.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Exceptions.h"
#include "FastaFileIndex.h"

using namespace BamTools;

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
		setDescription("Iteratively left-aligns and merges the insertions and deletions in all alignments.");
		addInfile("in", "Input BAM file.", false, true);
		addOutfile("out", "Output BAM file.", false, true);
		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("max_iter", "Maximum number of iterations per read.", true, 50);
		addFlag("v", "Verbose mode: Prints changed/unstable alignment names to the command line.");
	}

	virtual void main()
	{
		//init
		int max_iter = getInt("max_iter");
		bool verbose = getFlag("v");
		QTextStream out(stdout);

		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");
		FastaFileIndex reference(ref_file);

		//open BAM reader
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("in"));

		//open BAM writer
		BamWriter writer;
		if (!writer.Open(getOutfile("out").toStdString(), reader.GetHeaderText(), reader.GetReferenceData()))
		{
			THROW(CommandLineParsingException, "Could not open output file '" + getOutfile("out") + "' for writing!");
		}

		//align
		long unchanged = 0;
		long changed = 0;
		long unstable = 0;
		BamAlignment alignment;
		while (reader.GetNextAlignment(alignment))
		{
			// skip unmapped alignments, as they cannot be left-realigned without CIGAR data
			if (alignment.IsMapped())
			{
				//This should not happen - kept because it was in original code...
                Chromosome chr(reader.GetReferenceData()[alignment.RefID].RefName);
				int length = alignment.GetEndPosition() - alignment.Position + 1;
				if (alignment.Position < 0 || length <= 0)
				{
                    THROW(ArgumentException, "Invalid aligment found in BAM file '" + getInfile("in") + "': " + QString::fromStdString(alignment.Name) + " (" + chr.str() + ":" + QString::number(alignment.Position) + ")");
				}

				//get reference sequence
				std::string ref_seq = reference.seq(chr, alignment.Position+1, length).constData();
				if (ref_seq=="")
				{
                    THROW(ArgumentException, "Empty reference sequence returned for chromosomal position '" + chr.str() + ":" + QString::number(alignment.Position) + "' with length '" + QString::number(length) + "!");
				}

				int iter = 0;
				while (iter<max_iter)
				{
					if (!leftAlign(alignment, ref_seq)) break;
					++iter;
				}

				if (iter==0)
				{
					++unchanged;
				}
				else if (iter==max_iter)
				{
					++unstable;
                    if (verbose) out << "Unstable alignment: " << QString::fromStdString(alignment.Name) << " (pos: " << chr.str() << ":" << alignment.Position << ")" << endl;
				}
				else
				{
					++changed;
                    if (verbose) out << "Changed alignment: " << QString::fromStdString(alignment.Name)  << " (pos: " << chr.str() << ":" << alignment.Position << ", iterations: " << iter << ")" << endl;
				}
			}

			writer.SaveAlignment(alignment);
		}

		//close reader/writer
		reader.Close();
		writer.Close();

		//summary output
		long all = unchanged + changed + unstable;
		out << "Mapped alignments  : " << all << endl;
		out << "Changed alignments : " << changed << " (" << QString::number(100.0*changed/all, 'f', 2) << "%)" << endl;
		out << "Unstable alignments: " << unstable << " (" << QString::number(100.0*unstable/all, 'f', 2) << "%)" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
