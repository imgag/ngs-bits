#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include <QFile>

class ConcreteTool: public ToolBase {
    Q_OBJECT

private:
    QByteArray getId(QByteArray *header) {
        auto start = header->indexOf("ID=") + 3;
        auto end = header->indexOf(',', start);
        QByteArray id;
        for (int i = start; i < end; ++i) {
            id.append(header->at(i));
        }
        return id;
    }

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
        // TODO: When QT fixes its shit we can use std::map, see https://bugreports.qt.io/browse/QTCREATORBUG-18536
        QMap<QByteArray, std::string> informations; // which ID will be REF or ALT

        while(!in_p->atEnd())
        {
            QByteArray line = in_p->readLine();

            //skip empty lines
            if (line.trimmed().isEmpty()) continue;

            //write out headers unchanged
            if (line.startsWith('#'))
            {
                out_p->write(line);
                if ((line.contains("Number=R") || line.contains("Number=A")) && line.startsWith("##INFO"))
                {
                    auto id = getId(&line);
                    informations[id] = (line.contains("Number=R")) ? "R" : "A";
                }
                continue;
            }

            //split line and extract variant infos
            QByteArrayList parts = line.split('\t');
            if (parts.count()<static_cast<int>(MANDATORY_ROWS::LENGTH)) THROW(FileParseException, "VCF with too few columns: " + line);

            if (!parts[static_cast<int>(MANDATORY_ROWS::ALT)].contains(',')) // ignore because no allele
            {
                out_p->write(line);
			}
			else
            {
                QByteArrayList alt = parts[static_cast<int>(MANDATORY_ROWS::ALT)].split(',');
                QByteArrayList info = parts[static_cast<int>(MANDATORY_ROWS::INFO)].split(';');

                // Checks wether or not any ALT or REF is contained in this line
                std::vector<bool> infoTypes;
                auto containsREF = false;
                auto containsALT = false;

                for (int i = 0; i < info.length(); ++i) {
                    auto rows = info[i].split('=');
                    auto containsId = informations.find(rows[0]);
                    if (containsId != informations.end())
                    {
                        auto infoType = informations[rows[0]];
                        if (infoType == "R")
                        {
                            containsREF = true;
                        }
                        else
                        {
                            containsALT = true;
                        }
                        infoTypes.push_back(true);
                    }
                    else
                    {
                        infoTypes.push_back(false);
                    }
                }

                // Skips line if no ALT or REF could be found
                if (!containsREF && !containsALT)
                {
                    out_p->write(line);
                    continue;
                }

                // Creates needed lines
                // n (allele) for ALT or n+1 for REF
                std::vector<QByteArray> lines;

                if (containsREF)
                {
                    lines.push_back(line); // include the original line for reference
                }


                // For each allele construct a separate info block
                QByteArrayList constructedInfos;
                for (int i = 0; i < alt.length(); ++i) {
                    QByteArray empty;
                    constructedInfos.append(empty);
                }

                for (int i = 0; i < info.length(); ++i)
                {
                    // If type is ALT OR REF split by HEADER / VALUE and assign every line a different value (starting by 0-index)
                    if (infoTypes[i])
                    {
                        auto rows = info[i].split('='); // split HEADER=VALUE
                        auto infoPerAllele = rows[1].split(',');

                        if (infoPerAllele.size() != alt.size()) THROW(FileParseException, "VCF contains missmatch of info columns with multiple alleles: " + line);

                        for (int j = 0; j < constructedInfos.size(); ++j) {
                            // appends a HEADER=VALUE; (with semicolon)
                            auto constructedInfo = infoPerAllele[j].prepend(rows[0] + "=");
                            if (j != (constructedInfos.size() - 1)) {
                                constructedInfo.append(';');
                            }

                            constructedInfos[j].push_back(constructedInfo);
                        }

                    }
                    else
                    {

                        for (int j = 0; j < constructedInfos.size(); ++j)
                        {
                            constructedInfos[j].push_back(info[i]);
                        }
                    }
                }

                // Iterate through alleles and construct a new QByteArrayList from the parts and designated constructedInfos
                // Then join to a QByteArray and write
                for (int allel = 0; allel < alt.size(); ++allel)
                {
                    QByteArrayList lineParts = parts;
                    lineParts[static_cast<int>(MANDATORY_ROWS::ALT)] = alt[allel];
                    lineParts[static_cast<int>(MANDATORY_ROWS::INFO)] = constructedInfos[allel];
                    lines.push_back(lineParts.join('\t'));
                }

                for (int l = 0; l < lines.size(); ++l) {
                    out_p->write(lines[l]);
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
