#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include <QFile>

class ConcreteTool: public ToolBase
{
    Q_OBJECT

private:
	QByteArray getId(const QByteArray& header)
	{
		auto start = header.indexOf("ID=") + 3;
		auto end = header.indexOf(',', start);
		return header.mid(start, end-start);
    }

    /*!
     * \brief Searches a string for seperator before column n
     * \param text - the text to search for
     * \param seperator - the seperator to look for: usually ,
     * \param colum - the column to look in e.g start looking for in the 4th colum
     */
	bool includesSeperator(const QByteArray& text, const char& seperator, int column)
	{
		auto current_col = 0;
        for (int i = 0; i < text.length(); ++i)
        {
			if (text[i] == '\t')
            {
				++current_col;
            }

			if (current_col == column)
            {
                if (text[i] == seperator)
                {
					return true;
                }
            }

			if (current_col > column) break;
        }

		return false;
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
                else if (line.startsWith("##FORMAT") && (line.contains("Number=R") || line.contains("Number=A")))
                {
					auto id = getId(line);
					format2type[id] = line.contains("Number=R") ? R : A;
                }
                continue;
            }

			if (!includesSeperator(line, ',', static_cast<int>(MANDATORY_ROWS::ALT))) // ignore because no allele  //TODO use const int
			{
                out_p->write(line);
			}
			else
            {
                //split line and extract variant infos
				QByteArrayList parts = line.trimmed().split('\t');
				if (parts.length() < static_cast<int>(MANDATORY_ROWS::LENGTH)) THROW(FileParseException, "VCF with too few columns: " + line);

                QByteArrayList alt = parts[static_cast<int>(MANDATORY_ROWS::ALT)].split(',');
                QByteArrayList info = parts[static_cast<int>(MANDATORY_ROWS::INFO)].split(';');
                QByteArrayList format = parts[static_cast<int>(MANDATORY_ROWS::LENGTH)].split(':');

                // Checks wether or not any ALT or REF is contained in this line
				std::vector<AnnotationType> info_types;
                std::vector<AnnotationType> format_types;
				bool contains_multiallelic_info = false;
				bool contains_multiallelic_format = false;

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

				for (int i = 0; i < format.length(); ++i)
				{
                    if (format2type.contains(format[i]))
                    {
                        format_types.push_back(format2type[format[i]]);
                        contains_multiallelic_info = true;
						contains_multiallelic_format = true;
                    }
                    else
                    {
                        format_types.push_back(OTHER);
                    }
                }

                if (!contains_multiallelic_info)
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

						int parts_expected = alt.size() + (info_types.at(i)==R);
						if (info_value_per_allele.size() != parts_expected) THROW(FileParseException, "VCF contains missmatch of info columns with multiple alleles (expected " + QByteArray::number(parts_expected) + ", got " + QByteArray::number(info_value_per_allele.size()) + "): " + line);

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

                // For each sample in the line construct a new sample according to the format for every allele
                std::vector<std::vector<QByteArray>> new_samples_per_allele;

				if (contains_multiallelic_format)
                {
                    // initialize SAMPLE_COUNT new QByteArrays for every allele
                    auto samples_count = parts.length() - 1 - static_cast<int>(MANDATORY_ROWS::LENGTH);
                    for (int i = 0; i < alt.length(); ++i)
                    {
                        new_samples_per_allele.push_back(std::vector<QByteArray>(samples_count));
                    }

                    // for every sample part in the specific sample process REF or ALT
                    // then append to new_samples_per_allele[ALLEL][SAMPLE_INDEX]
                    for (int i = 0; i < samples_count; ++i)
                    {
                        auto sample_column = static_cast<int>(MANDATORY_ROWS::LENGTH) + i + 1;
                        QByteArrayList sample_values = parts[sample_column].split(':');

                        for (int j = 0; j < sample_values.length(); ++j)
                        {
                            if (format_types.at(j) == R || format_types.at(j) == A)
							{
								auto sample_value_parts = sample_values[j].split(',');

								int parts_expected = alt.size() + (format_types.at(j)==R);
								if (sample_value_parts.size() != parts_expected) THROW(FileParseException, "VCF contains invalid element count in format entry " + QByteArray::number(j+1) + " of sample " + QByteArray::number(i+1) + " (expected " + QByteArray::number(parts_expected) + ", got " + QByteArray::number(sample_value_parts.size()) + "): " + line);

								for (int a = 0; a < alt.length(); ++a)
								{
									if (!new_samples_per_allele[a][i].isEmpty()) new_samples_per_allele[a][i] += ":";

									// appends a VALUE: (with seperator)
									if (format_types[j] == R)
									{
										new_samples_per_allele[a][i] += sample_value_parts[0];
										new_samples_per_allele[a][i] += ',';
										new_samples_per_allele[a][i] += sample_value_parts[a+1];
									}
									else
									{
										new_samples_per_allele[a][i] += sample_value_parts[a];
									}
								}
                            }
                            else
                            {
                                for (int a = 0; a < alt.length(); ++a)
                                {
                                    if (!new_samples_per_allele[a][i].isEmpty()) new_samples_per_allele[a][i] += ":";
                                    new_samples_per_allele[a][i] += sample_values[j];
                                }
                            }
                        }
                    }
                }

                // Iterate through alleles and construct a new QByteArrayList from the parts and designated constructedInfos
				// Then join to a QByteArray and write
                for (int allel = 0; allel < alt.size(); ++allel)
                {
					parts[static_cast<int>(MANDATORY_ROWS::ALT)] = alt[allel];
					parts[static_cast<int>(MANDATORY_ROWS::INFO)] = new_infos_per_allele[allel];
					if (contains_multiallelic_format)
                    {
                        for (unsigned int i = 0; i < new_samples_per_allele[allel].size(); ++i)
                        {
                            auto part_index = static_cast<int>(MANDATORY_ROWS::LENGTH) + 1 + i;
                            parts[part_index] = new_samples_per_allele[allel][i];
                        }
                    }
					out_p->write(parts.join('\t').append('\n'));
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
