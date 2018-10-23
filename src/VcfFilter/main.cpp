#include "Exceptions.h"
#include "ToolBase.h"
#include "BedFile.h"
#include "VcfFile.h"
#include "ChromosomalIndex.h"
#include "Helper.h"
#include <QFile>

class ConcreteTool: public ToolBase
{
    Q_OBJECT

    /**
     * @brief Returns the parts by column
     * @param line
     * @param column
     * @return
     */
    QByteArray getPartByColumn (const QByteArray line, int column)
    {
        int columns_seen = 0;
        int column_start, column_end;

        for (int i = 0; i < line.length(); ++i)
        {
            if (columns_seen == column + 1)
            {
                break;
            }
            else if (line[i] ==  '\t')
            {
                ++columns_seen;
                if (columns_seen == column)
                {
                    column_start = i;
                }
                else if (columns_seen == column + 1)
                {
                    column_end = i;
                }
            }
        }

        return line.mid(column_start, column_end - column_start);
    }

    /**
     * @brief Returns how many columns exist in this VCF
     * @param line
     * @return
     */
    int countColumns (const QByteArray line)
    {
        int columns_seen = 0;
        for (int i = 0; i < line.length(); ++i)
        {
            if (line[i] == '\t') ++columns_seen;
        }
        return columns_seen;
    }

    /**
     * @brief Checks wether or not the predicate filter is satisified by value. e.g 7, >= 5
     * @param value
     * @param filter_op
     * @param filter_val
     * @return bool
     */
    bool satisfiesFilter(const QString value, const QString filter_op, const QString filter_val)
    {
        bool pass_filter = false;
        if (filter_op == '>')
        {
            pass_filter = filter_val.toDouble() > value.toDouble();
        }
        else if (filter_op == "<=")
        {
            pass_filter = filter_val.toDouble() >= value.toDouble();
        }
        else if (filter_op == '=')
        {
            pass_filter = filter_val.toDouble() == value.toDouble();
        }
        else if (filter_op == "<=")
        {
            pass_filter = filter_val.toDouble() <= value.toDouble();
        }
        else if (filter_op == '<')
        {
            pass_filter = filter_val.toDouble() < value.toDouble();
        }
        else if (filter_op == "is")
        {
            pass_filter = filter_val == value;
        }
        else if (filter_op == "contains")
        {
            pass_filter = value.contains(filter_val);
        }
        else
        {
            THROW(ProgrammingException, "Invalid filter operation " + filter_op + "!");
        }

        return pass_filter;
    }

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)

    {
        operations << ">" << ">=" << "=" << "<=" << "<" << "is" << "contains";
        variant_types << "snp" << "complex" << "other";
    }

    QStringList operations;
    QStringList variant_types;

    virtual void setup()
    {
		setDescription("Filters a VCF based on the given criteria.");
        //optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
        addString("reg", "Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.", true);
        addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
        addFloat("qual", "Filters the VCF entries by minimum quality", true);
        addFlag("filter_empty", "Removes all empty filters");
        addString("filter", "Filters the VCF entries by filter regex", true);
        addString("id", "Filters the VCF entries by ID regex", true);
        addEnum("variant_type", "Filters by variant type", true, variant_types);
        addString("info_filter", "Filters info with operators, e.g. 'depth > 17'.\nValid operations are '" + operations.join("','") + "'.", true);
        addString("sample_filters", "Filters samples with operators, e.g. 'depth > 17'.\nValid operations are '" + operations.join("','") + "'.", true);

		//TODO reg: BED/comma-separated regions
        //quality:min
        //filter_empty ('PASS', '', '.')
        //filter:regexp
        //id:regexp
        //variant_type: SNV,INDEL,OTHER > addEnum();
        //info:complex e.g. 'AO > 2;DP > 10' (==,>=,<=,<,>,!=, contains,is)
		//TODO sample: complex

		changeLog(2018, 10, 31, "Initial implementation.");
    }

    virtual void main()
    {
		//init
		QString reg = getInfile("reg");

		//load target region
		BedFile roi;
		if (QFile::exists(reg))
		{
			roi.load(reg);
		}
		else //parse comma-separated regions
		{
			//TODO
		}
		roi.merge();
        ChromosomalIndex<BedFile> roi_index(roi);

        //open input/output streams
        QString in = getInfile("in");
        QString out = getOutfile("out");
        if(in!="" && in==out)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }
        QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
        QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

        // Read input
        QByteArrayList lines;
        QByteArrayList headers;
        while (!in_p->atEnd())
        {
            lines.push_back(in_p->readLine());
        }
        in_p->close();

        ///Remove our headers
        std::remove_copy_if(lines.begin(), lines.end(), lines.begin(), [](QByteArray line) { return line.startsWith('#'); });

        ///Filter by region. First return CHROM and POS and then remove all lines that do not satisfy the check
        if (reg != "")
        {
            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
               auto chr = getPartByColumn(line, VcfFile::CHROM);
               auto start = getPartByColumn(line, VcfFile::POS);
               auto ref = getPartByColumn(line, VcfFile::REF);

               return roi_index.matchingIndex(chr, start.toInt(), start.toInt() + ref.length());
            });
        }

        ///Filter by QUALITY. Return QUAL and then remove all lines that do not satisfy the check
        double quality = getFloat("qual");
        if (quality != 0.0)
        {
            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
                auto qual = getPartByColumn(line, VcfFile::QUAL);

                return qual.toDouble() < quality;
            });
        }

        ///Filter by empty filters (will remove empty filters).
        bool filter_empty = getFlag("filter_empty");
        if (filter_empty)
        {
            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
                auto filter = getPartByColumn(line, VcfFile::FILTER);

                return filter == "." || filter == "," || filter == "PASS";
            });
        }

        ///Filter FILTER column via regex
        QString filter_regex = getString("filter");
        if (filter_regex != "")
        {
            QRegExp re(filter_regex);
            if (!re.isValid())
            {
                THROW(ArgumentException, filter_regex + " is not a valid regular expression!");
            }

            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
                return !re.indexIn(line);
            });
        }

        ///Filter ID column via regex
        QString id_regex = getString("id");
        if (id_regex != "")
        {
            QRegExp re(id_regex);
            if (re.isValid())
            {
                THROW(ArgumentException, id_regex + "is not a valid regular expression!");
            }

            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
                return !re.indexIn(line);
            });
        }

        ///Filter by type and or info operators in INFO column
        QString variant_type = getString("variant_type");
        QString info_filter = getString("info_filter");
        if (variant_type != "" || info_filter != "")
        {
            QStringList filters;
            if (variant_type != "") filters.append("TYPE =" + variant_type);
            if (info_filter != "") filters + info_filter.split(';');

            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
                auto info_parts = getPartByColumn(line, VcfFile::INFO).split(';');
                bool passes_filters = true;

                for (int i = 0; i < filters.size(); ++i)
                {
                    if (!passes_filters) break; // We already failed previously
                    auto filter_parts = filters[i].split(' ');

                    for (int j = 0; j < info_parts.length(); ++j)
                    {
                        auto mid_index = info_parts[j].indexOf('=');
                        auto info_id = info_parts[j].left(mid_index);
                        if (info_id == filter_parts[0])
                        {
                           bool passes = satisfiesFilter(info_parts[j].right(info_parts[j].length() - mid_index), filter_parts[1], filter_parts[2]);
                           if (!passes)
                           {
                            passes_filters = false;
                            break;
                           }
                        }
                    }
                }

                return passes_filters;
            });

        }

        ///Filter by sample operators in the SAMPLE column
        QString sample_filter = getString("sample_filter");
        if (sample_filter != "")
        {
            QStringList filters = sample_filter.split(';');
            auto column_index = VcfFile::MIN_COLS + 1;
            auto column_count = countColumns(headers[headers.length() - 1]); // it might happen that lines is already empty, so better make sure to count the format line

            std::remove_if(lines.begin(), lines.end(), [&](QByteArray line)
            {
                auto format_ids = getPartByColumn(line, VcfFile::FORMAT).split(':');
                std::vector<QByteArrayList> sample_parts; // create a list of qbytearray for every sample in this line
                for (int i = column_index; i < column_count; ++i)
                {
                    auto sample = getPartByColumn(line, i);
                    sample_parts.push_back(sample.split(':'));
                }

                bool passes_filters = true;
                for (int i = 0; i < filters.length(); ++i) // for every filter check every id
                {
                    if (!passes_filters) break;
                    auto filter_parts = filters[i].split(' ');

                    int index = format_ids.indexOf(filter_parts[0].toUtf8());
                    if (index >= -1) // if ID is contained check in all samples that the filter passes
                    {
                        for (unsigned int j = 0; j < sample_parts.size(); ++j)
                        {
                            auto passes = satisfiesFilter(sample_parts[j][index], filter_parts[1], filter_parts[2]);
                            if (!passes)
                            {
                                passes_filters = false;
                                break;
                            }
                        }
                    }
                }

                return passes_filters;
            });
        }

        // Always write the headers unchanged
        for (int i = 0; i < headers.length(); ++i)
        {
            out_p->write(headers[i]);
        }
        // Write the remaining lines
        for (int i = 0; i < lines.length(); ++i)
        {
            out_p->write(lines[i]);
        }
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
