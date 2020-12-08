#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "FastqFileStream.h"
#include "BedFile.h"
#include "Settings.h"
#include "FastaFileIndex.h"
#include <chrono>

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
		setDescription("Paired-end read simulator for Illumina reads.");
		setExtendedDescription(QStringList() << "PERsim generates paired-end reads of a given length for a region of interest in the genome:"
											 << " - insert size is modelled using a gaussian distribution."
											 << " - read-through into the sequencing adapters is modelled."
											 << " - sequencing errors are modelled using a simple uniform distribution."
							   );
		addInfile("roi", "Target region BED file.", false);
		addInt("count", "Number of read pairs to generate.", false);
		addOutfile("out1", "Forward reads output file in .FASTQ.GZ format.", false);
		addOutfile("out2", "Reverse reads output file in .FASTQ.GZ format.", false);
		//optional
		addInt("length", "Read length for forward/reverse reads.", true, 100);
		addInt("ins_mean", "Library insert size mean value.", true, 200);
		addInt("ins_stdev", "Library insert size mean standard deviation.", true, 70);
		addFloat("error", "Base error probability (uniform distribution).", true, 0.01);
		addInt("max_n", "Maximum number of N bases (from reference genome).", true, 5);
		addString("a1", "Forward read sequencing adapter sequence (for read-through).", true, "AGATCGGAAGAGCACACGTCTGAACTCCAGTCACGAGTTA");
		addString("a2", "Reverse read sequencing adapter sequence (for read-through).", true, "AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGTAGATCTC");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("v", "Enable verbose debug output.");
		addInt("compression_level", "Output FASTQ compression level from 1 (fastest) to 9 (best compression).", true, Z_BEST_SPEED);

		changeLog(2020, 7, 15, "Added 'compression_level' parameter.");
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		int compression_level = getInt("compression_level");
		FastqOutfileStream out1(getOutfile("out1"), compression_level);
		FastqOutfileStream out2(getOutfile("out2"), compression_level);
		BedFile roi;
		roi.load(getInfile("roi"));
		long count = getInt("count");
		int length = getInt("length");
		int ins_mean = getInt("ins_mean");
		int ins_stdev = getInt("ins_stdev");
		QString a1 = getString("a1");
		QString a2 = getString("a2");
		bool verbose = getFlag("v");
		double error = getFloat("error");
		int max_n = getInt("max_n");

		//set qualities
		QByteArray qualities = "AAAAAFFFFFFFGGGGGGFFFFFFFFFFEEEEEEEEEEEEEEEEEEEEEDDDDDDDDDDDDDDDDDDDDDDDDCCCCCCCCCCCCCCCCCCBBBBBBBBBBBBBBBBBBBBBBBBBAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
		qualities = qualities.left(length);

		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

		//normal distribution for insert size
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::mt19937 gen(seed);
		std::normal_distribution<> insert_dist(ins_mean, ins_stdev);

		//determine overall ROI size
		roi.merge();
		double overall_bases = roi.baseCount();

		//simulate reads (for each target region)
		for (int r=0; r<roi.count(); ++r)
		{
			const BedLine& reg = roi[r];
			const Chromosome& chr = reg.chr();

			//determine how many reads to generate for this region
			long r_count = std::round((double)count * reg.length() / overall_bases);
			out << "Region: " << chr.str() << ":" << reg.start() << "-" << reg.end() << " length=" << reg.length() << " read_pairs=" << r_count << endl;
			int skipped_n = 0;
			for (long i=0; i<r_count; ++i)
			{
				//determine insert size
				int is = std::max((int)insert_dist(gen), 0);

				//determine start and end position (the read should overlap with the region)
				std::uniform_int_distribution<int> start_dist(reg.start()-is/2, reg.end()-is/2);
				int start = start_dist(gen);
				if (start<0) start=0;
				int end = start + is;
				if (end>reference.lengthOf(chr)) end = reference.lengthOf(chr);

				//get insert sequence;
				Sequence seq = reference.seq(chr, start, end-start);
				if (verbose) out << "  Insert: " << chr.str() << ":" << start << "-" << (end-1) << " length=" << seq.length() << endl;

				//get read seqence
				FastqEntry r1;
				FastqEntry r2;
				r1.bases = seq.left(length);
				r2.bases = seq.right(length);
				r2.bases.reverseComplement();

				//skip read pairs with too many N bases
				if (r1.bases.count('N')>max_n || r2.bases.count('N')>max_n)
				{
					++skipped_n;
					continue;
				}

				//append sequencing adapers (if too short)
				int a_length = length - r1.bases.length();
				if (a_length>0)
				{
					r1.bases.append(a1.left(a_length));
					r2.bases.append(a2.left(a_length));
				}

				//append random sequence with overrepresended 'A' (if still too short)
				int r_length = length - r1.bases.length();
				r1.bases.append(Helper::randomString(r_length, "AACGT"));
				r2.bases.append(Helper::randomString(r_length, "AACGT"));

				//add noise
				int ec1 = r1.bases.addNoise(error, gen);
				int ec2 = r2.bases.addNoise(error, gen);
				if (verbose) out << "    Read 1: errors=" << ec1 << " seq=" << r1.bases << endl;
				if (verbose) out << "    Read 2: errors=" << ec2 << " seq=" << r2.bases << endl;

				//set headers and qualities
				r1.header = "@PERsim:1:ABCDEF:1:" + chr.str() + ":" + QByteArray::number(start) + ":" + QByteArray::number(end-1) + " 1:N:0:AACGTGAT";
				r2.header = "@PERsim:1:ABCDEF:1:" + chr.str() + ":" + QByteArray::number(start) + ":" + QByteArray::number(end-1) + " 2:N:0:AACGTGAT";
				r1.header2 = "+";
				r2.header2 = "+";
				r1.qualities = qualities;
				r2.qualities = qualities;

				//validate data
				r1.validate();
				r2.validate();
				if (r1.bases.length()!=length) THROW(ProgrammingException, "Read 1 is only " + QString::number(r1.bases.length()) + " bases long!");
				if (r2.bases.length()!=length) THROW(ProgrammingException, "Read 2 is only " + QString::number(r2.bases.length()) + " bases long!");

				//write output
				out1.write(r1);
				out2.write(r2);
			}

			if (skipped_n>0) out << "  Skipped read pairs (too many Ns): " << skipped_n << endl;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
