#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"
#include <QFile>

class ConcreteTool: public ToolBase
{
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
		setDescription("Breaks multi-allelic variants into several lines, making sure that allele-specific INFO/SAMPLE fields are still valid.");
        //optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
        addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addFlag("no_errors", "Ignore VCF format errors if possible.");
		addFlag("verbose", "Writes ignored VCF format errors to stderr.");

		changeLog(2018, 10, 18, "Initial implementation.");
    }

	///Return ID from FORMAT/INFO line
	QByteArray getId(const QByteArray& header)
	{
		int start = header.indexOf("ID=") + 3;
		int end = header.indexOf(',', start);
		return header.mid(start, end-start);
	}

	/*!
	 * \brief Searches a string for seperator before column n
	 * \param text - the text to search for
	 * \param seperator - the seperator to look for: usually ,
	 * \param colum - the column to look in e.g start looking for in the 4th colum
	 */
	bool includesSeperator(const QByteArray& text, char seperator, int column)
	{
		int current_col = 0;
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

    virtual void main()
    {
        //open input/output streams
        QString in = getInfile("in");
        QString out = getOutfile("out");
		bool no_errors = getFlag("no_errors");
		bool verbose = getFlag("verbose");
        if(in!="" && in==out)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }
        QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		enum AnnotationType {R, A, OTHER};
		QHash<QByteArray, AnnotationType> info2type;
		QHash<QByteArray, AnnotationType> format2type;
		QHash<QByteArray, int> ignored_info_field_errors;
		QHash<QByteArray, int> ignored_format_field_errors;
		QTextStream out_stream(stderr);

        while(!in_p->atEnd())
        {
            QByteArray line = in_p->readLine();

            //skip empty lines
            if (line.trimmed().isEmpty()) continue;

			//header
            if (line.startsWith('#'))
			{
				if (line.startsWith("##INFO") && (line.contains("Number=R") || line.contains("Number=A")))
				{
					info2type[getId(line)] = line.contains("Number=R") ? R : A;
                }
                else if (line.startsWith("##FORMAT") && (line.contains("Number=R") || line.contains("Number=A")))
				{
					format2type[getId(line)] = line.contains("Number=R") ? R : A;
                }

				out_p->write(line);
                continue;
            }

			//single-allele variant > write out unchanged
			if (!includesSeperator(line, ',', VcfFile::ALT))
			{
                out_p->write(line);
				continue;
			}

			//split line and extract variant infos
			QByteArrayList parts = line.trimmed().split('\t');
			if (parts.length() < VcfFile::MIN_COLS) THROW(FileParseException, "VCF with too few columns: " + line);

			QByteArrayList alt = parts[VcfFile::ALT].split(',');
			QByteArrayList info = parts[VcfFile::INFO].split(';');
			bool has_samples = parts.count()> VcfFile::MIN_COLS;
			QByteArrayList format;
			if (has_samples) format = parts[VcfFile::FORMAT].split(':');

			// For each allele construct a separate info block
			QVector<QByteArray> new_infos_per_allele(alt.length());
			for (int i = 0; i < info.length(); ++i)
			{
				QByteArrayList info_parts = info[i].split('='); // split HEADER=VALUE
				const QByteArray& info_name = info_parts[0];

				// If type is ALT OR REF split by HEADER / VALUE and assign every line a different value (starting by 0-index)
				if (info2type.contains(info_name))
				{
					AnnotationType type = info2type[info_name];
					QByteArrayList info_value_per_allele = info_parts[1].split(',');

					int parts_expected = alt.size() + (type==R);
					if (info_value_per_allele.size() != parts_expected)
					{
						if (no_errors)
						{
							for (int j = 0; j < new_infos_per_allele.size(); ++j)
							{
								if (!new_infos_per_allele[j].isEmpty()) new_infos_per_allele[j] += ";";
								new_infos_per_allele[j] += info[i];
							}
							if (verbose)
							{
								ignored_info_field_errors[info_name] += 1;
							}
						}
						else
						{
							THROW(FileParseException, "VCF INFO field '" + info_name + "' has wrong number of elements (expected " + QByteArray::number(parts_expected) + ", got " + QByteArray::number(info_value_per_allele.size()) + "): " + line);
						}
					}
					else
					{
						for (int j = 0; j < new_infos_per_allele.size(); ++j)
						{
							// appends a HEADER=VALUE; (with semicolon)
							if (!new_infos_per_allele[j].isEmpty()) new_infos_per_allele[j] += ";";
							if (type==R) // use INFO (ref), ALLELE (count)
							{
								new_infos_per_allele[j] += info_name + '=' + info_value_per_allele[0] + ',' + info_value_per_allele[j+1];
							}
							else // use ALLELE (count)
							{
								new_infos_per_allele[j] += info_name + '=' + info_value_per_allele[j];
							}
						}
					}

				}
				else
				{
					for (int j = 0; j < new_infos_per_allele.size(); ++j)
					{
						if (!new_infos_per_allele[j].isEmpty()) new_infos_per_allele[j] += ";";
						new_infos_per_allele[j] += info[i];
					}
				}
			}

			QVector<QVector<QByteArray>> new_samples_per_allele;
			if (has_samples)
			{
				QVector<AnnotationType> format_types;
				for (int i = 0; i < format.length(); ++i)
				{
					if (format2type.contains(format[i]))
					{
						format_types.push_back(format2type[format[i]]);
					}
					else
					{
						format_types.push_back(OTHER);
					}
				}

				// For each sample, construct a new sample according to the format for every allele
				int samples_count = parts.length() - VcfFile::FORMAT - 1;
				for (int i = 0; i < alt.length(); ++i)
				{
					new_samples_per_allele.push_back(QVector<QByteArray>(samples_count));
				}

				// for every sample part in the specific sample process REF or ALT
				// then append to new_samples_per_allele[ALLEL][SAMPLE_INDEX]
				for (int i = 0; i < samples_count; ++i)
				{
					int sample_column = VcfFile::FORMAT + i + 1;
                    if (parts[sample_column] == ".") {
                        continue; // Skip MISSING sample
                    }

					QByteArrayList sample_values = parts[sample_column].split(':');
					for (int j = 0; j < sample_values.length(); ++j)
					{
						if (j==0 && format[j]=="GT") //special handling GT entry (must be first entry if present!)
						{
							//comma is not valid
							if (sample_values[j].contains(',')) THROW(FileParseException, "VCF contains invalid GT entry for sample #" + QByteArray::number(i+1) + ": " + line);
							//only one allele (chrX/Y for males) or two alleles is valid, e.g. 0, 1, 0/1, 0|1, ...
							if (sample_values[j].count()!=1 && sample_values[j].count()!=3) THROW(FileParseException, "VCF contains invalid GT entry for sample #" + QByteArray::number(i+1) + ": " + line);

							//check for phased GT
							bool phased = sample_values[j].contains('|');

							for (int a = 0; a < alt.length(); ++a)
							{
								int allele_count = sample_values[j].count(QByteArray::number(a+1));
								int wt_count = sample_values[j].count('0');
								if (allele_count==0 && wt_count==2)
								{
									if (phased)
									{
										new_samples_per_allele[a][i] = "0|0";
									}
									else new_samples_per_allele[a][i] = "0/0";
								}
								else if (allele_count==0 && wt_count==1)
								{
									if (phased)
									{
										if (sample_values[j].startsWith('0')) new_samples_per_allele[a][i] = "0|.";
										else new_samples_per_allele[a][i] = ".|0";
									}
									else new_samples_per_allele[a][i] = "./0";
								}
								else if (allele_count==0 && wt_count==0)
								{
									if (phased)
									{
										new_samples_per_allele[a][i] = ".|.";
									}
									else new_samples_per_allele[a][i] = "./.";
								}
								else if (allele_count==1 && wt_count==1)
								{
									if (phased)
									{
										if (sample_values[j].startsWith('0')) new_samples_per_allele[a][i] = "0|1";
										else new_samples_per_allele[a][i] = "1|0";
									}
									else new_samples_per_allele[a][i] = "0/1";
								}
								else if (allele_count==1 && wt_count==0)
								{
									if (phased)
									{
										if (sample_values[j].startsWith(QByteArray::number(a+1))) new_samples_per_allele[a][i] = "1|.";
										else new_samples_per_allele[a][i] = ".|1";
									}
									else new_samples_per_allele[a][i] = "./1";
								}
								else //allele_count==2 && wt_count==0
								{
									if (phased)
									{
										new_samples_per_allele[a][i] = "1|1";
									}
									else new_samples_per_allele[a][i] = "1/1";
								}
							}
						}
						else if (sample_values[j] == ".")
						{
							for (int a = 0; a < alt.length(); ++a)
							{
								if (!new_samples_per_allele[a][i].isEmpty()) new_samples_per_allele[a][i] += ":";
								new_samples_per_allele[a][i] += sample_values[j];
							}
						}

						else if (format_types.at(j) == R || format_types.at(j) == A) //special handling A/R entries
						{
							QByteArrayList sample_value_parts = sample_values[j].split(',');

							int parts_expected = alt.size() + (format_types.at(j)==R);
							if (sample_value_parts.size() != parts_expected)
							{
								if (no_errors)
								{
									for (int a = 0; a < alt.length(); ++a)
									{
										if (!new_samples_per_allele[a][i].isEmpty()) new_samples_per_allele[a][i] += ":";
										new_samples_per_allele[a][i] += sample_values[j];
									}
									if (verbose)
									{
										ignored_format_field_errors[format[j]] += 1;
									}
								}
								else
								{
									THROW(FileParseException, "VCF contains invalid element count in format entry " + format[j] + " for sample #" + QByteArray::number(i+1) + " (expected " + QByteArray::number(parts_expected) + ", got " + QByteArray::number(sample_value_parts.size()) + "): " + line);
								}
							}
							else
							{
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
						}
						else //other entries > write out unchanged
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
			for (int a = 0; a < alt.size(); ++a)
			{
				parts[VcfFile::ALT] = alt[a];
				parts[VcfFile::INFO] = new_infos_per_allele[a];
				if (has_samples)
				{
					for (int i = 0; i < new_samples_per_allele[a].size(); ++i)
					{
						int part_index = VcfFile::FORMAT + 1 + i;
						parts[part_index] = new_samples_per_allele[a][i];
					}
				}
				out_p->write(parts.join('\t').append('\n'));
			}
		}

		//Output ignored errors
		if (no_errors && verbose)
		{
			for (auto it=ignored_info_field_errors.begin(); it!=ignored_info_field_errors.end(); ++it)
			{
				out_stream << "Ignored invalid value count of INFO field '" << it.key() << "' " << QString::number(it.value()) << " times" << endl;
			}
			for (auto it=ignored_format_field_errors.begin(); it!=ignored_format_field_errors.end(); ++it)
			{
				out_stream << "Ignored invalid value count of FORMAT field '" << it.key() << "' " << QString::number(it.value()) << " times" << endl;
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
