#include "ToolBase.h"
#include "VersatileFile.h"
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
		setDescription("Replaces sample identifiers in the VCF header.");
		setExtendedDescription(QStringList() << "Note: the sample ID matching is performed case-sensitive.");
		addString("ids", "Comma-separated list of sample ID pairs in the format 'old1=new1,old2=new2,...'.", false);
		//optional
		addInfile("in", "Input variant list in VCF or VCF.GZ format. If unset, reads from STDIN.", true);
		addOutfile("out", "Output variant list in VCF format. If unset, writes to STDOUT.", true);
		addInt("compression_level", "Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.", true, BGZF_NO_COMPRESSION);

		changeLog(2025,  8, 27, "Initial version.");
	}


	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if (in!="" && in==out) THROW(ArgumentException, "Parameters 'in' and 'out' cannot be the same file!");
		QSharedPointer<QFile> out_file = Helper::openFileForWriting(out, true);

		//parse sample replacement
		using IdPair=QPair<QByteArray,QByteArray>;
		QList<IdPair> ids;
		foreach(QString entry, getString("ids").split(","))
		{
			int pos = entry.indexOf("=");
			if (pos==-1) THROW(ArgumentException, "Parameter 'ids' contains entry without '=': " + entry);

			ids.append(QPair<QByteArray, QByteArray>(entry.left(pos).trimmed().toUtf8(), entry.mid(pos+1).trimmed().toUtf8()));
		}

		//define line beginnings of lines to replace
		QList<QByteArray> markers;
		markers << "##SAMPLE=";
		markers << "#CHROM\t";
		markers << "##DRAGENCommandLine=";
		markers << "##GATKCommandLine=";
		markers << "##cmdline="; //STRELKA
		markers << "##commandline="; //freebayes

		//process
		VersatileFile file(in, true);
		file.open(QFile::ReadOnly|QFile::Text);
		while(!file.atEnd())
		{
			QByteArray line = file.readLine(false);

			foreach(const QByteArray& marker, markers)
			{
				if(line.startsWith(marker))
				{
					foreach(const IdPair& pair, ids)
					{
						line.replace(pair.first, pair.second);
					}
				}
			}

			out_file->write(line);
		}
		file.close();
		out_file->close();
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

