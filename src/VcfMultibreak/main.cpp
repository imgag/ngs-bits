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
        QMap<QByteArray, std::string> formats;

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
                else if ((line.contains("Number=R") || line.contains("Number=A")) && line.startsWith("##FORMATS"))
                {
                    auto id = getId(&line);
                    formats[id] = (line.contains("Number=R")) ? "R" : "A";
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
                QByteArrayList format = parts[static_cast<int>(MANDATORY_ROWS::LENGTH)].split(':');

                // Checks wether or not any ALT or REF is contained in this line
                std::vector<std::string> infoTypes;
                bool containsREForALT = false;

                for (int i = 0; i < info.length(); ++i) {
                    auto rows = info[i].split('=');
                    auto containsInfo = informations.find(rows[0]);

                    if (containsInfo != informations.end()) {
                        infoTypes.push_back(containsInfo.value());
                        containsREForALT = true;
                    }
                    else
                    {
                        infoTypes.push_back("");
                    }
                }

                if (!containsREForALT) {
                    // this also needs to respect format
                    out_p->write(line);
                    continue;
                }

                // For each allele construct a separate info block
                std::vector<QByteArray> constructedInfos(alt.length());

                for (int i = 0; i < info.length(); ++i)
                {
                    // If type is ALT OR REF split by HEADER / VALUE and assign every line a different value (starting by 0-index)
                    if (infoTypes.at(i) == "A" || infoTypes.at(i) == "R")
                    {
                        auto rows = info[i].split('='); // split HEADER=VALUE
                        auto infoPerAllele = rows[1].split(',');

                        auto mandatorySize = (infoTypes.at(i) == "R") ? alt.size() + 1 : alt.size();
                        if (infoPerAllele.size() != mandatorySize) THROW(FileParseException, "VCF contains missmatch of info columns with multiple alleles: " + line);

                        for (int j = 0; j < constructedInfos.size(); ++j) {
                            // appends a HEADER=VALUE; (with semicolon)
                            auto constructedInfo = infoPerAllele[j].prepend(rows[0] + "=");
                            if (i != (info.length() - 1)) {
                                constructedInfo.append(';');
                            }

                            constructedInfos[j].push_back(constructedInfo);
                        }

                    }
                    else
                    {

                        for (int j = 0; j < constructedInfos.size(); ++j)
                        {
                            auto constructedInfo = info[i];
                            if (i != (info.length() - 1)) {
                                constructedInfo.append(';');
                            }
                            constructedInfos[j].push_back(constructedInfo);
                        }
                    }
                }

                // Iterate through alleles and construct a new QByteArrayList from the parts and designated constructedInfos
                // Then join to a QByteArray and write
                std::vector<QByteArray> lines;
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
