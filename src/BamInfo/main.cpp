#include "ToolBase.h"
#include "BamReader.h"
#include <QFile>
#include <QFileInfo>

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
        setDescription("Basic BAM information.");
        addInfileList("in", "Input BAM/CRAM files.", false);
        //optional
        addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
        addFlag("name", "Add filename only to output. The default is to add the canonical file path.");
        addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

        changeLog(2025,  9,  6, "First version.");
	}

	virtual void main()
	{
        //init
        QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
        QString ref = getInfile("ref");
        bool name = getFlag("name");

        //write header
        QByteArrayList headers;
        headers << "filename" << "format" << "genome_build" << "genome_masked" << "genome_contains_alt" << "mapper" << "paired-end";
        out->write("#" + headers.join("\t") + "\n");

        QStringList in = getInfileList("in");
        foreach(QString filename, in)
        {
            BamReader reader(filename, getInfile("ref"));
            BamInfo info = reader.info();
            QByteArrayList parts;
            QFileInfo file_info(filename);
            parts << (name ? file_info.fileName() : file_info.canonicalFilePath()).toUtf8();
            parts << info.file_format;
            parts << info.build;
            parts << (info.false_duplications_masked ? "yes" : "no");
            parts << (info.contains_alt_chrs ? "yes" : "no");
            parts << (info.mapper + " " + info.mapper_version).trimmed();
            parts << (info.paired_end ? "yes" : "no");

            out->write(parts.join("\t") + "\n");
            out->flush();
        }
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
