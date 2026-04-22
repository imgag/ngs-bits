#include "ToolBase.h"
#include "BasicStatistics.h"
#include "FastaFileIndex.h"
#include "Settings.h"
#include "Exceptions.h"
#include "VcfFile.h"
#include <cmath>
#include "BamReader.h"

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
		setDescription("Annotates VCF variants with allele frequency from a BAM/CRAM file.");
		addInfile("in", "Input variant list to annotate in VCF(.GZ) format.", false, true);
		addInfile("bam", "Input BAM/CRAM file.", false, true);
		addOutfile("out", "Output variant list file in VCF format.", false, true);
		//optional
		addFlag("depth", "Annotate an additional INFO field entry containing the depth.");
		addString("name", "INFO field entry prefix in output file.", true, "N");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);

		changeLog(2025,   11, 28, "Initial version.");
	}

	virtual void main()
	{
		//init
		bool depth = getFlag("depth");
		QString ref_file = getInfile("ref");
		QByteArray name = getString("name").toUtf8();
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//load input
		VcfFile input;
		input.setAllowMultiSample(false);
		input.load(getInfile("in"));
		BamReader reader(getInfile("bam"), ref_file);

		//determine frequencies and depths
		FastaFileIndex reference(ref_file);
		for (int i = 0; i < input.count(); ++i)
		{
			QByteArrayList info_keys;
			QByteArrayList info_values;
			const Variant& variant(input[i]);
			VariantDetails tmp = reader.getVariantDetails(reference, variant, false);

			info_keys.append(name + "_AF");

			//annotate variant
			if (tmp.depth==0 || !BasicStatistics::isValidFloat(tmp.frequency))
			{
				info_values.append("0");
			}
			else
			{
				info_values.append(QByteArray::number(tmp.frequency, 'f', 4));
			}
			if (depth)
			{
				info_keys.append(name + "_DP");
				info_values.append(QByteArray::number(tmp.depth));
			}

			//get existing infos
			foreach (QByteArray key, input[i].infoKeys())
			{
				info_keys.append(key);
				info_values.append(input[i].info(key));
			}

			input[i].setInfo(info_keys, info_values);
		}

		//add header descriptions for each new INFO
		QByteArray sample;
		if (name == "N") sample = "normal sample";
		else sample = name;

		InfoFormatLine info_line;
		info_line.description = "Variant allele frequency in " + sample;
		info_line.id = name + "_AF";
		info_line.number = "1";
		info_line.type = "Float";
		input.vcfHeader().addInfoLine(info_line);

		if (depth)
		{
			InfoFormatLine info_line;
			info_line.description = "Read depth in " + sample;
			info_line.id = name + "_DP";
			info_line.number = "1";
			info_line.type = "Integer";
			input.vcfHeader().addInfoLine(info_line);
		}

		//store
		input.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

