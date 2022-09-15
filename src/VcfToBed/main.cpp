#include "ToolBase.h"
#include "Helper.h"
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
		setDescription("Converts a VCF file to a BED file.");
		setExtendedDescription(QStringList() << "For SNVs exactly one base is in the output region."
											 << "For insertions the base before and after the insertion is contained."
											 << "For deletions the base before the deletion and all deleted bases are contained.");
		addInfile("in", "Input variant list in VCF format.", true, true);
		addOutfile("out", "Output region in BED format.", true, true);
		//optional
		addFlag("add_chr", "Add 'chr' to chromosome names if missing.");

		changeLog(2022,  9, 13, "Initial implementation.");
	}

	virtual void main()
	{
		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		bool add_chr = getFlag("add_chr");

		//process VCF
		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine().trimmed();

			//skip empty lines and header/comment lines
			if (line.isEmpty() || line.startsWith('#')) continue;

			//check column count
			QByteArrayList parts = line.split('\t');
			if (parts.count()<VcfFile::MIN_COLS)
			{
				THROW(FileParseException, "VCF line with less than 8 fields found: '" + line.trimmed() + "'");
			}

			//extract relevant infos
			QByteArray chr = parts[VcfFile::CHROM];
			if (add_chr && !chr.toLower().startsWith("chr"))
			{
				chr.prepend("chr");
			}
			int pos = Helper::toInt(parts[VcfFile::POS], "chromosomal position");
			QByteArray ref = parts[VcfFile::REF];

			//write output
			out_p->write(chr + "\t" + QByteArray::number(pos-1) + "\t" + QByteArray::number(pos+ref.length()-1) + "\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
