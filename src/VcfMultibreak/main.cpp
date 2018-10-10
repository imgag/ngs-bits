#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QFile>

class ConcreteTool: public ToolBase {
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
        setDescription("If multiple alleles are specified in a single record, break the record into multiple lines, preserving allele-specific INFO fields.");
        //optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
        addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

        changeLog(2018, 10, 8, "Initial implementation.");
    }

    virtual void main()
    {
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

            if (!parts[4].contains(',')) { // ignore because no allele
                out_p->write(line);
            } else {
                QList<QByteArray> alt = parts[4].split(',');
                QList<QByteArray> info = parts[7].split(';');
            }
        }
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
