#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include <QFile>

class ConcreteTool: public ToolBase {
    Q_OBJECT

private:
	QByteArray getId(const QByteArray& header)
	{
		auto start = header.indexOf("ID=") + 3;
		auto end = header.indexOf(',', start);
		return header.mid(start, end-start);
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
		enum AnnotationType {R, A, OTHER};
		QMap<QByteArray, AnnotationType> info2type; // which ID will be REF or ALT
		QMap<QByteArray, AnnotationType> format2type;

        while(!in_p->atEnd())
        {
            QByteArray line = in_p->readLine();

            //skip empty lines
            if (line.trimmed().isEmpty()) continue;

            //write out headers unchanged
            if (line.startsWith('#'))
            {
                out_p->write(line);
				if (line.startsWith("##INFO") && (line.contains("Number=R") || line.contains("Number=A")))
                {
					auto id = getId(line);
					info2type[id] = line.contains("Number=R") ? R : A;
                }
				else if (line.startsWith("##FORMATS") && (line.contains("Number=R") || line.contains("Number=A")))
                {
					auto id = getId(line);
					format2type[id] = line.contains("Number=R") ? R : A;
                }
                continue;
            }

            //split line and extract variant infos
            QByteArrayList parts = line.split('\t');
			if (parts.count()<static_cast<int>(MANDATORY_ROWS::LENGTH)) THROW(FileParseException, "VCF with too few columns: " + line); //TODO const int

			if (!parts[static_cast<int>(MANDATORY_ROWS::ALT)].contains(',')) // ignore because no allele //TODO use method with O(1) complexity
            {
                out_p->write(line);
			}
			else
            {
                QByteArrayList alt = parts[static_cast<int>(MANDATORY_ROWS::ALT)].split(',');
                QByteArrayList info = parts[static_cast<int>(MANDATORY_ROWS::INFO)].split(';');
                QByteArrayList format = parts[static_cast<int>(MANDATORY_ROWS::LENGTH)].split(':');

                // Checks wether or not any ALT or REF is contained in this line
				std::vector<AnnotationType> info_types;
				bool contains_multiallelic_info = false;

				for (int i = 0; i < info.length(); ++i)
				{
					QByteArray info_name = info[i].split('=')[0];
					if (info2type.contains(info_name))
					{
						info_types.push_back(info2type[info_name]);
						contains_multiallelic_info = true;
                    }
                    else
                    {
						info_types.push_back(OTHER);
                    }
                }

				if (!contains_multiallelic_info)  // this also needs to respect format
				{
                    out_p->write(line);
                    continue;
                }

                // For each allele construct a separate info block
				std::vector<QByteArray> new_infos_per_allele(alt.length());

                for (int i = 0; i < info.length(); ++i)
                {
                    // If type is ALT OR REF split by HEADER / VALUE and assign every line a different value (starting by 0-index)
					if (info_types.at(i) == A || info_types.at(i) == R)
                    {
						auto info_parts = info[i].split('='); // split HEADER=VALUE
						auto info_value_per_allele = info_parts[1].split(',');

						if (info_value_per_allele.size() != alt.size() + (info_types.at(i)==R)) THROW(FileParseException, "VCF contains missmatch of info columns with multiple alleles: " + line);

						for (unsigned int j = 0; j < new_infos_per_allele.size(); ++j)
						{
							// appends a HEADER=VALUE; (with semicolon)
							if (!new_infos_per_allele[j].isEmpty()) new_infos_per_allele[j] += ";";
							if (info_types.at(i) == R) // use INFO (ref), ALLELE (count)
							{
								new_infos_per_allele[j] += info_parts[0] + '=' + info_value_per_allele[0] + ',' + info_value_per_allele[j+1];
							}
							else // use ALLELE (count)
							{
								new_infos_per_allele[j] += info_parts[0] + '=' + info_value_per_allele[j];
							}
                        }

                    }
                    else
                    {
						for (unsigned int j = 0; j < new_infos_per_allele.size(); ++j)
						{
							if (!new_infos_per_allele[j].isEmpty()) new_infos_per_allele[j] += ";";
							new_infos_per_allele[j] += info[i];
                        }
                    }
                }

                // Iterate through alleles and construct a new QByteArrayList from the parts and designated constructedInfos
				// Then join to a QByteArray and write
                for (int allel = 0; allel < alt.size(); ++allel)
                {
					parts[static_cast<int>(MANDATORY_ROWS::ALT)] = alt[allel];
					parts[static_cast<int>(MANDATORY_ROWS::INFO)] = new_infos_per_allele[allel];
					out_p->write(parts.join('\t'));
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
