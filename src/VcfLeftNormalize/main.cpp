#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VcfFile.h"
#include "Settings.h"
#include <QFile>
#include <QTextStream>
#include <QList>

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
		setDescription("Normalizes all variants and shifts indels to the left in a VCF file. Multi-allelic and complex variant are not changed!");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF or VCF or VCF.GZ file. If unset, writes to STDOUT.", true, true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("compression_level", "Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.", true, BGZF_NO_COMPRESSION);
		addFlag("stream", "Allows to stream the input and output VCF without loading the whole file into memory. Only supported with uncompressed VCF files.");
		addFlag("right", "Right normalize vcf. Reverse direction and shift indels to the right.");

		changeLog(2020, 8, 12, "Added parameter '-compression_level' for compression level of output vcf files.");
		changeLog(2016, 06, 24, "Initial implementation.");
	}

	void streamVcf(const QString& in, const QString& out, const QString& ref_file)
	{
		FastaFileIndex reference(ref_file);
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//write out headers unchanged
			if (line.startsWith('#'))
			{
				out_p->write(line);
				continue;
			}

			//split line and extract variant infos
			QList<QByteArray> parts = line.split('\t');
			if (parts.count()<5) THROW(FileParseException, "VCF with too few columns: " + line);

			Chromosome chr = parts[0];
			int pos = Helper::toInt(parts[1], "VCF position");
			Sequence ref = parts[3].toUpper();
			QByteArray alt = parts[4].toUpper();

			//write out multi-allelic variants unchanged
			if (alt.contains(','))
			{
				writeLine(out_p, parts, pos, ref, alt);
				continue;
			}

			QList<Sequence> alts;
			alts.append(alt);

			VcfLine vcf_line = VcfLine(chr, pos, ref, alts);

			if (! getFlag("right"))
			{
				vcf_line.leftNormalize(reference, true);
			}
			else
			{
				vcf_line.rightNormalize(reference, true);
			}

			writeLine(out_p, parts, vcf_line.start(), vcf_line.ref(), vcf_line.alt()[0]); // only one alt value allowed

		}
	}


	void writeLine(QSharedPointer<QFile>& out_p, const QList<QByteArray>& parts, int pos, const QByteArray& ref, const QByteArray& alt)
	{
		char tab = '\t';
		out_p->write(parts[0]);
		out_p->write(&tab, 1);
		out_p->write(QByteArray::number(pos));
		out_p->write(&tab, 1);
		out_p->write(parts[2]);
		out_p->write(&tab, 1);
		out_p->write(ref);
		out_p->write(&tab, 1);
		out_p->write(alt);
		for(int i=5; i<parts.count(); ++i)
		{
			out_p->write(&tab, 1);
			out_p->write(parts[i]);
		}
	}

	virtual void main()
	{
		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		int compression_level = getInt("compression_level");

		if (getFlag("stream")) //This code streams input and output without keeping the whole VCF in memory which allows to normalize large VCFs.
		{	
			streamVcf(in, out, ref_file);
		}
		else
		{
			VcfFile vcf_file;
			vcf_file.load(in);
			if (!getFlag("right"))
			{
				vcf_file.leftNormalize(ref_file);
			}
			else
			{
				vcf_file.rightNormalize(ref_file);
			}
			vcf_file.store(out, true, compression_level);
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

