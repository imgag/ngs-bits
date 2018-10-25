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
        if (filter_op == ">")
        {
            pass_filter = filter_val.toDouble() > value.toDouble();
        }
        else if (filter_op == "<=")
        {
            pass_filter = filter_val.toDouble() >= value.toDouble();
        }
        else if (filter_op == "!=")
        {
            pass_filter = filter_val.toDouble() != value.toDouble();
        }
        else if (filter_op == "=")
        {
            pass_filter = filter_val.toDouble() == value.toDouble();
        }
        else if (filter_op == "<=")
        {
            pass_filter = filter_val.toDouble() <= value.toDouble();
        }
        else if (filter_op == "<")
        {
            pass_filter = filter_val.toDouble() < value.toDouble();
        }
        else if (filter_op == "is")
        {
            pass_filter = filter_val == value;
        }
        else if (filter_op == "not")
        {
            pass_filter = filter_val != value;
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
        operations << ">" << ">=" << "=" << "!=" << "<=" << "<" << "is" << "not" << "contains";
        variant_types << "snp" << "complex" << "other";
    }

    QStringList operations;
    QStringList variant_types;

    virtual void setup()
    {
		setDescription("Filters a VCF based on the given criteria.");
        //optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		addString("reg", "Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.", true);
        addString("variant_type", "Filters by variant type. Possible types are: " + variant_types.join("','"), true);
		addString("id", "Filters the VCF entries by ID regex", true);
        addFloat("qual", "Filters the VCF entries by minimum quality", true);
        addFlag("filter_empty", "Removes all empty filters");
		addString("filter", "Filters the VCF entries by filter regex", true);
		addString("info_filter", "Filters info with operators - use ';' as separator for several filters. E.g. 'DP > 5;AO > 2'.\nValid operations are '" + operations.join("','") + "'.", true);
		addString("sample_filter", "Filters samples with operators - use ';' as separator for several filters. E e.g. 'GT is 1/1'.\nValid operations are '" + operations.join("','") + "'.", true);

		changeLog(2018, 10, 31, "Initial implementation.");
    }

    virtual void main()
    {
        //init roi
        QString reg = getString("reg");

		//load target region
		BedFile roi;
		if (QFile::exists(reg))
		{
			roi.load(reg);
		}
		else //parse comma-separated regions
		{
            auto regions = reg.split(',');
			foreach(QString region, regions)
			{
				BedLine line = BedLine::fromString(region);
				if (!line.isValid()) THROW(ArgumentException, "Invalid region '" + region + "' given in parameter 'reg'!");
				roi.append(line);
			}
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

        //init parameters
        double quality = getFloat("qual");
        bool filter_empty = getFlag("filter_empty");
        QString filter_regex = getString("filter");
        QString id_regex = getString("id");
        QString variant_type = getString("variant_type");
        QString info_filter = getString("info_filter");
        QString sample_filter = getString("sample_filter");

        QRegExp filter_re;
        if (filter_regex != "")
        {
            // Prepare static filter regexes
            filter_re.setPattern(filter_regex);
            if (!filter_re.isValid())
            {
				THROW(ArgumentException, "Filter regexp '" + filter_regex + "' is not a valid regular expression!");
            }
        }

		QRegExp id_re;
        if (id_regex != "")
        {
			id_re.setPattern(id_regex);
			if (id_re.isValid())
            {
                THROW(ArgumentException, "ID regexp '" + id_regex + "' is not a valid regular expression!");
            }
        }

        QRegExp operator_regex("(\\w*)\\s(>|>=|=|!=|<=|<|is|not|contains)\\s(\\w*)");
        // Set up filter lists for info
        //QStringList info_filters;
        std::vector<QStringList> info_filters;
        if (info_filter != "")
        {
            if (info_filter != "")
            {
                auto info_filter_split = info_filter.split(';');
                for (int i = 0; i < info_filter_split.length(); ++i)
                {
                    if (!operator_regex.indexIn(info_filter_split[i]))
                    {
                        QStringList matches = operator_regex.capturedTexts();
                        info_filters.push_back(matches.mid(1, matches.length()));
                    }
                    else
                    {
                        THROW(ArgumentException, "Operation '" + info_filter_split[i] + "' is invalid!");
                    }
                }
            }
        }

        // Set up filter lists and additional info for samples
        auto column_index = VcfFile::MIN_COLS + 1;
        auto column_count = 0;
        std::vector<QStringList> sample_filters;
        auto sample_filter_split = sample_filter.split(';');
        for (int i = 0; i < sample_filter_split.length(); ++i)
        {
            if (!operator_regex.indexIn(sample_filter_split[i]))
            {
                QStringList matches = operator_regex.capturedTexts();
                sample_filters.push_back(matches.mid(1, matches.length()));
            }
            else
            {
                THROW(ArgumentException, "Operation '" + sample_filter_split[i] + "' is invalid!");
            }
        }

        // Read input
        while (!in_p->atEnd())
        {
            auto line = in_p->readLine();

            if (line.startsWith("#CHROM"))
            {
                column_count = countColumns(line);
            }
            if (line.startsWith('#'))
            {
                out_p->write(line);
                continue;
            }

            ///Filter by region. First return CHROM and POS and then remove all lines that do not satisfy the check
            if (reg != "")
            {
                auto chr = getPartByColumn(line, VcfFile::CHROM);
                auto start = getPartByColumn(line, VcfFile::POS);
                auto ref = getPartByColumn(line, VcfFile::REF);

                if (!roi_index.matchingIndex(chr, start.toInt(), start.toInt() + ref.length()))
                {
                    continue;
                }
            }

            ///Filter by variant_type.
            if (variant_type != "")
            {
                // NOTE: This is calculated with
                // len(ref)==1&&len(alt)==1 > SNP
                // len(ref)>1||len(alt)>1 > INDEL
                // '<' > OTHER,
                auto ref = getPartByColumn(line, VcfFile::REF);
                auto alt = getPartByColumn(line, VcfFile::ALT);

                QString type;
                if (ref.length() == 1 && alt.length() == 1)
                {
                    type = "snp";
                }
                else if (ref.length() > 1 || alt.length() > 1)
                {
                    type = "complex";
                }
                else if (alt.startsWith('<'))
                {
                    type = "other";
                }
                else
                {
                    THROW(ProgrammingException, "Unsupported variant type '" + alt + "' in line " + line);
                }

                if (type != variant_type)
                {
                    continue;
                }

            }

            ///Filter by QUALITY. Return QUAL and then remove all lines that do not satisfy the check
            if (quality != 0.0)
            {
                auto qual = getPartByColumn(line, VcfFile::QUAL);
                bool is_convertable;

                if (qual.toDouble(&is_convertable) < quality)
                {
                    if (!is_convertable && qual != ".") THROW(ProgrammingException, "'" + qual + "' is an unsupported quality value in line: " + line);
                    continue;
                }
            }

            ///Filter by empty filters (will remove empty filters).
            if (filter_empty)
            {
                auto filter = getPartByColumn(line, VcfFile::FILTER);

				if (filter == "." || filter == "" || filter == "PASS")
                {
                    continue;
                }
            }

            ///Filter FILTER column via regex
            if (filter_regex != "")
            {
                if (!filter_re.indexIn(line))
                {
                    continue;
                }
            }

            ///Filter ID column via regex
            if (id_regex != "")
            {
				if (!id_re.indexIn(line))
                {
                    continue;
                }
            }

            ///Filter by type and or info operators in INFO column
            if (info_filters.size())
            {
                auto info_parts = getPartByColumn(line, VcfFile::INFO).split(';');
                bool passes_filters = true;

                for (unsigned int i = 0; i < info_filters.size(); ++i)
                {
                    if (!passes_filters) break; // We already failed previously
                    auto filter_parts = info_filters[i];

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

                if (!passes_filters)
                {
                    continue;
                }
            }

            ///Filter by sample operators in the SAMPLE column
            if (sample_filters.size())
            {
                auto format_ids = getPartByColumn(line, VcfFile::FORMAT).split(':');
                std::vector<QByteArrayList> sample_parts; // create a list of QByteArray for every sample in this line
                for (int i = column_index; i < column_count; ++i)
                {
                    auto sample = getPartByColumn(line, i);
                    sample_parts.push_back(sample.split(':'));
                }

                bool passes_filters = true;
                for (unsigned int i = 0; i < sample_filters.size(); ++i) // for every filter check every id
                {
                    if (!passes_filters) break;
                    auto filter_parts = sample_filters[i];

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

                if (!passes_filters)
                {
                    continue;
                }
            }

            out_p->write(line);

        }

        // Close streams
        in_p->close();
        out_p->close();
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
