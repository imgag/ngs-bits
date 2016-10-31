#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
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
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);

		changeLog(2016, 06, 24, "Initial implementation.");
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

	void normalize(int& start, Sequence& ref, Sequence& alt)
	{
		//remove common prefix
		while(ref.length()>0 && alt.length()>0 && ref[0]==alt[0])
		{
			ref = ref.mid(1);
			alt = alt.mid(1);
			start += 1;
		}

		//remove common suffix
		while(ref.length()>0 && alt.length()>0 && ref[ref.length()-1]==alt[alt.length()-1])
		{
			ref.resize(ref.length()-1);
			alt.resize(alt.length()-1);
		}
	}

	virtual void main()
	{
		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");
		FastaFileIndex reference(ref_file);

		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
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
			int pos = Helper::toInt(parts[1], "variant position");
			Sequence ref = parts[3].toUpper();
			Sequence alt = parts[4].toUpper();

			//write out multi-allelic variants unchanged
			if (alt.contains(','))
			{
				writeLine(out_p, parts, pos, ref, alt);
				continue;
			}

			//write out SNVs unchanged
			if (ref.length()==1 && alt.length()==1)
			{
				writeLine(out_p, parts, pos, ref, alt);
				continue;
			}

			//skip all variants starting at first base of chromosome
			if (pos==1)
			{
				writeLine(out_p, parts, pos, ref, alt);
				continue;
			}

			//skip SNVs disguised as indels (e.g. ACGT => AXGT)
			normalize(pos, ref, alt);
			if (ref.length()==1 && alt.length()==1)
			{
				writeLine(out_p, parts, pos, ref, alt);
				continue;
			}

			//skip complex indels (e.g. ACGT => CA)
			if (ref.length()!=0 && alt.length()!=0)
			{
				writeLine(out_p, parts, pos, ref, alt);
				continue;
			}

			//left-align INSERTION
			if (ref.length()==0)
			{
				//shift block to the left
				Sequence block = Variant::minBlock(alt);
				pos -= block.length();
				while(pos>0 && reference.seq(chr, pos, block.length())==block)
				{
					pos -= block.length();
				}
				pos += block.length();

				//prepend prefix base
				pos -= 1;
				ref = reference.seq(chr, pos, 1);
				alt = ref + alt;

				//shift single-base to the left
				while(ref[0]==alt[alt.count()-1])
				{
					pos -= 1;
					ref = reference.seq(chr, pos, 1);
					alt = ref + alt.left(alt.length()-1);
				}
			}

			//left-align DELETION
			else
			{
				//shift block to the left
				Sequence block = Variant::minBlock(ref);
				while(pos>=1 && reference.seq(chr, pos, block.length())==block)
				{
					pos -= block.length();
				}
				pos += block.length();

				//prepend prefix base
				pos -= 1;
				alt = reference.seq(chr, pos, 1);
				ref = alt + ref;

				//shift single-base to the left
				while(ref[ref.count()-1]==alt[0])
				{
					pos -= 1;
					alt = reference.seq(chr, pos, 1);
					ref = alt + ref.left(ref.length()-1);
				}
			}

			writeLine(out_p, parts, pos, ref, alt);
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

