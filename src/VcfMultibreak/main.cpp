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
		setDescription("Breaks multi-allelic variants in several lines, preserving allele-specific INFO/SAMPLE fields.");
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
			QByteArrayList parts = line.split('\t');
			if (parts.count()<8) THROW(FileParseException, "VCF with too few columns: " + line);

            if (!parts[4].contains(',')) // ignore because no allele
            {
                out_p->write(line);
			}
			else
            {
				QByteArrayList alt = parts[4].split(',');
				QByteArrayList info = parts[7].split(';');

				QList<QByteArrayList> infoPerAllele; // handles the information per allele
				for (int allel = 0; allel < alt.size(); ++allel)
				{
					QByteArrayList allelInfo;
					for (int i = 0; i < info.size(); ++i)
					{
                        QByteArray empty;
                        allelInfo.append(empty);
                    }
                    infoPerAllele.append(allelInfo);
                }

                QByteArray concreteInfo;
				QByteArrayList rows;
				QByteArrayList derivedInformations;
                for (int i = 0; i < info.length(); ++i)
                {
                    concreteInfo = info[i];

                    if (concreteInfo.contains(','))
                    {
                        rows = concreteInfo.split('='); // split HEADER=VALUE
                        derivedInformations = rows[1].split(',');
                        if (derivedInformations.size() != alt.size()) THROW(FileParseException, "VCF contains missmatch of info columns with multiple alleles: " + line);

                        for (int l = 0; l < alt.size(); l++) {
                            infoPerAllele[l][i].append(derivedInformations[l].prepend(rows[0] + "=")); // include header
                        }
                    } else
                    {
                        for(int j = 0; j < alt.size(); ++j)
                        {
                            infoPerAllele[j][i].append(concreteInfo);
                        }
                    }
                }

                char tab = '\t';
                for (int allel = 0; allel < alt.size(); ++allel) {
                    for (int part = 0; part < parts.size(); ++part) {
                        if (part == 4) {
                            out_p->write(alt[allel]);
                        } else if (part == 5) {
                            for (int i = 0; i < infoPerAllele[allel].size(); ++i) {
                                out_p->write(infoPerAllele[allel][i]);
                            }
                        } else {
                            out_p->write(parts[part]);
                        }

                        if (part != parts.size() - 1) {
                            out_p->write(&tab, 1);
                        }
                    }
                }

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
