#include "ToolBase.h"
#include "VcfFile.h"

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
		setDescription("Determines mendelian error rate from a trio VCF.");
		addInfile("vcf", "Multi-sample VCF or VCF.GZ file.", false);
		addString("c", "Sample name of child in VCF.", false);
		addString("f", "Sample name of father in VCF.", false);
		addString("m", "Sample name of mother in VCF.", false);

		//optional
		addOutfile("out", "Output text file. If unset, writes to STDOUT.", true);
		addInt("min_dp", "Minimum depth in each sample.", true, 0);
		addFloat("min_qual", "Minimum QUAL of variants.", true, 0.0);
		addFlag("debug", "Enable debug output");

		//changelog
		changeLog(2025,  2, 18, "Initial version of the tool.");
	}

	enum Genotype
	{
		HOM,
		HET,
		WT,
		UNKNOWN,
		INVALID
	};

	Genotype genotype(const QByteArrayList& parts, int index)
	{
		QByteArray format = parts[index];
		int i_sep = format.indexOf(':');
		QByteArray gt = format.left(i_sep);

		//normalize
		gt.replace('|', '/');

		//convert
		if (gt=="1/1") return Genotype::HOM;
		else if (gt=="0/1" || gt=="1/0") return Genotype::HET;
		else if (gt=="0/0") return Genotype::WT;
		else if (gt.contains('.')) return Genotype::UNKNOWN;
		return Genotype::INVALID;
	}

	int depth(const QByteArrayList& parts, int index, int i_dp)
	{
		QByteArrayList format = parts[index].split(':');
		QByteArray dp = format[i_dp];
		bool ok = false;
		int dp_val = dp.toInt(&ok);

		if (!ok) return -1;

		return dp_val;
	}

	virtual void main()
    {
		//init
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QByteArray c = getString("c").toUtf8();
		QByteArray f = getString("f").toUtf8();
		QByteArray m = getString("m").toUtf8();
		int min_dp = getInt("min_dp");
		double min_qual = getFloat("min_qual");
		bool debug = getFlag("debug");

		//column indices
		int i_format = VcfFile::FORMAT;
		int i_c = -1;
		int i_f = -1;
		int i_m = -1;

		//output counts
		int c_vars_checked = 0;
		int c_vars_mer = 0;
		int c_skip_not_autosome = 0;
		int c_skip_multiallelic = 0;
		int c_skip_depth_low = 0;
		int c_skip_depth_invalid = 0;
		int c_skip_qual_low = 0;
		int c_skip_qual_invalid = 0;
		int c_skip_genotype_unknown = 0;
		int c_skip_genotype_invalid = 0;

		//TODO Marc: implement a file stream (gzipped or normal) that returns lines (full or split by separator) and replace all uses of zlib with this class
		//open stream
		QByteArray vcf = getInfile("vcf").toUtf8();
		FILE* instream = fopen(vcf.data(), "rb");
		if (!instream) THROW(FileAccessException, "Could not open VCF file " + vcf);
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (!file) THROW(FileAccessException, "Could not open VCF file " + vcf);

		const int buffer_size = 1048576; //1MB buffer
		char* buffer = new char[buffer_size];
		while(!gzeof(file))
		{
			// get next line
			char* char_array = gzgets(file, buffer, buffer_size);

			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(file, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading file '" + vcf + "': " + error_message);
				}
			}
			QByteArray line = QByteArray(char_array);

			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//skip empty lines
			if (line.isEmpty() || line.startsWith("##")) continue;

			QByteArrayList parts = line.split('\t');
			if (parts.length()<12) THROW(FileParseException, "VCF with too few columns: " + line);

			//header line > determine column indices
			if (line.startsWith('#'))
			{
				i_c = parts.indexOf(c);
				if (i_c==-1) THROW(FileParseException, "Could not find FORMAT column for sample '" + c + "'!");

				i_f = parts.indexOf(f);
				if (i_f==-1) THROW(FileParseException, "Could not find FORMAT column for sample '" + f + "'!");

				i_m = parts.indexOf(m);
				if (i_m==-1) THROW(FileParseException, "Could not find FORMAT column for sample '" + m + "'!");

				continue;
			}

			//check that FORMAT is ok
			if (!parts[i_format].startsWith("GT:")) THROW(FileParseException, "Invalid FORMAT column! GT is not first entry: '" + parts[i_format] + "'!");


			//only variants on autosomes
			if (!Chromosome(parts[0]).isAutosome())
			{
				++c_skip_not_autosome;
				continue;
			}

			//not muli-allelic variants
			if (parts[VcfFile::ALT].contains(','))
			{
				++c_skip_multiallelic;
				continue;
			}

			//filter for depth
			if (min_qual>0)
			{
				QString qual = parts[VcfFile::QUAL];
				if (qual!=".")
				{
					if (!Helper::isNumeric(qual))
					{
						++c_skip_qual_invalid;
						continue;
					}
					else if (qual.toDouble()<min_qual)
					{
						++c_skip_qual_low;
						continue;
					}
				}
			}

			//filter for depth
			if (min_dp>0)
			{
				int i_dp = parts[i_format].split(':').indexOf("DP");
				int dp_c = depth(parts, i_c, i_dp);
				int dp_f = depth(parts, i_f, i_dp);
				int dp_m = depth(parts, i_m, i_dp);

				//check if depth is valid
				if (dp_c==-1 || dp_f==-1 || dp_m==-1)
				{
					if (debug) out->write("DEBUG - invalid DP: " + line + "\n");
					++c_skip_depth_invalid;
					continue;
				}

				//check depth if high enough
				if (dp_c<min_dp || dp_f<min_dp || dp_m<min_dp)
				{
					++c_skip_depth_low;
					continue;
				}
			}

			//determine genotypes
			Genotype gt_c = genotype(parts, i_c);
			Genotype gt_f = genotype(parts, i_f);
			Genotype gt_m = genotype(parts, i_m);
			if (gt_c==Genotype::UNKNOWN || gt_f==Genotype::UNKNOWN || gt_m==Genotype::UNKNOWN)
			{
				++c_skip_genotype_unknown;
				continue;
			}
			if (gt_c==Genotype::INVALID || gt_f==Genotype::INVALID || gt_m==Genotype::INVALID)
			{
				++c_skip_genotype_invalid;
				continue;
			}

			//perform emndelian error check
			++c_vars_checked;

			//hom, hom => het/wt
			if (gt_f==Genotype::HOM && gt_m==Genotype::HOM && gt_c!=Genotype::HOM) ++c_vars_mer;
			//hom, x => wt
			else if ((gt_f==Genotype::HOM || gt_m==Genotype::HOM) && gt_c==Genotype::WT) ++c_vars_mer;
			//wt, x => hom
			else if ((gt_f==Genotype::WT || gt_m==Genotype::WT) && gt_c==Genotype::HOM) ++c_vars_mer;
			//wt, wt  => het/hom
			else if (gt_f==Genotype::WT && gt_m==Genotype::WT && gt_c!=Genotype::WT) ++c_vars_mer;
		}

		//output
		out->write("Skipped variants not on autosomes: " + QByteArray::number(c_skip_not_autosome) + "\n");
		out->write("Skipped variants with multi-allelic alt: " + QByteArray::number(c_skip_multiallelic) + "\n");
		if (min_dp>0)
		{
			out->write("Skipped variants with low depth: " + QByteArray::number(c_skip_depth_low) + "\n");
			out->write("Skipped variants for which no depth could be determined: " + QByteArray::number(c_skip_depth_invalid) + "\n");
		}
		if (min_qual>0)
		{
			out->write("Skipped variants with low quality: " + QByteArray::number(c_skip_qual_low) + "\n");
			out->write("Skipped variants for which no quality could be determined: " + QByteArray::number(c_skip_qual_invalid) + "\n");
		}
		out->write("Skipped variants with (partially) unknown genotype: " + QByteArray::number(c_skip_genotype_unknown) + "\n");
		out->write("Skipped variants with invalid genotype: " + QByteArray::number(c_skip_genotype_invalid) + "\n");
		out->write("\n");

		out->write("Variants checked: " + QByteArray::number(c_vars_checked) + "\n");
		out->write("Mendelian error rate: " + QByteArray::number(100.0*c_vars_mer/c_vars_checked, 'f', 2) + "%\n");
	}
};


#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

