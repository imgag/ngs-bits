#include "Exceptions.h"
#include "ToolBase.h"
#include "BedFile.h"
#include "VcfFile.h"
#include "ChromosomalIndex.h"
#include "Helper.h"
#include <QFile>
#include <QRegularExpression>

struct FilterDefinition
{
	QByteArray op;
	QByteArray value;

	bool isValid(const QByteArrayList& op_numeric, const QByteArrayList& op_string)
	{
		//check that operator is valid
		if (!op_numeric.contains(op) && !op_string.contains(op))
		{
			return false;
		}

		//check that value is numeric if operator is numeric
		if (op_numeric.contains(op))
		{
			bool ok;
			value.toDouble(&ok);
			if (!ok) return false;
		}

		return true;
	}
};

class ConcreteTool: public ToolBase
{
    Q_OBJECT

	//Returns the content of a column by index (tab-separated line)
	static QByteArray getPartByColumn(const QByteArray& line, int index)
    {
        int columns_seen = 0;
		int column_start = 0;
		int column_end = -1;

        for (int i = 0; i < line.length(); ++i)
		{
			if (line[i]=='\t')
            {
                ++columns_seen;
				if (columns_seen == index)
                {
                    column_start = i;
					column_end = line.length() -1; // for last column that is not followed by a tab
				}
				else if (columns_seen == index + 1)
                {
                    column_end = i;
					break;
                }
			}
        }

		if (column_end==-1)
		{
			THROW(ProgrammingException, "Cannot find column " + QByteArray::number(index) + " in line: " + line);
		}

        return line.mid(column_start, column_end - column_start);
    }

	// Checks if a filter is satisified
	static bool satisfiesFilter(const QByteArray& value, const QByteArray& filter_name, const FilterDefinition& filter_def, const QByteArray& line)
    {
		if (filter_def.op == ">")
        {
			return toDouble(value, filter_name, line) > filter_def.value.toDouble();
        }
		else if (filter_def.op == ">=")
        {
			return toDouble(value, filter_name, line) >= filter_def.value.toDouble();
        }
		else if (filter_def.op == "!=")
        {
			return toDouble(value, filter_name, line) != filter_def.value.toDouble();
        }
		else if (filter_def.op == "=")
        {
			return toDouble(value, filter_name, line) == filter_def.value.toDouble();
        }
		else if (filter_def.op == "<=")
        {
			return toDouble(value, filter_name, line) <= filter_def.value.toDouble();
        }
		else if (filter_def.op == "<")
        {
			return toDouble(value, filter_name, line) < filter_def.value.toDouble();
        }
		else if (filter_def.op == "is")
        {
			return filter_def.value == value;
        }
		else if (filter_def.op == "not")
        {
			return filter_def.value != value;
        }
		else if (filter_def.op == "contains")
        {
			return value.contains(filter_def.value);
        }

		THROW(ProgrammingException, "Unsupported filter operation " + filter_def.op + "!");
    }

	//Convert a value to a double
	static double toDouble(const QByteArray& value, const QByteArray& filter_name, const QByteArray& line)
	{
		bool ok;
		double output = value.toDouble(&ok);
		if (!ok) THROW(ArgumentException, "Cannot convert value '" + value + "' to number for filter '" + filter_name + "' in line: " + line);
		return output;
	}

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)

	{
		op_numeric << ">" << ">=" << "=" << "!=" << "<=" << "<";
		op_string << "is" << "not" << "contains";
		variant_types << "snp" << "indel" << "multi-allelic" << "other";
    }

	QByteArrayList op_numeric;
	QByteArrayList op_string;
    QStringList variant_types;

    virtual void setup()
    {
		setDescription("Filters a VCF based on the given criteria.");
        //optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		addString("reg", "Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.", true);
		addString("variant_type", "Filters by variant type. Possible types are: " + variant_types.join("','") + ".", true);
		addString("id", "Filter by ID column (regular expression).", true);
		addFloat("qual", "Filter by QUAL column (minimum).", true);
		addString("filter", "Filter by FILTER column (regular expression).", true);
		addFlag("filter_empty", "Removes entries with non-empty FILTER column.");
		addString("info", "Filter by INFO column entries - use ';' as separator for several filters, e.g. 'DP > 5;AO > 2' (spaces are important).\nValid operations are '" + op_numeric.join("','") + "','" + op_string.join("','") + "'.", true);
		addString("sample", "Filter by sample-specific entries - use ';' as separator for several filters, e.g. 'GT is 1/1' (spaces are important).\nValid operations are '" + op_numeric.join("','") + "','" + op_string.join("','") + "'.", true);
        addFlag("sample_one_match", "If sample_one_match is active samples are in OR mode. They will pass a filter once one or more of the sample passes the filters.");

		changeLog(2018, 10, 31, "Initial implementation.");
    }

    virtual void main()
    {
        //init roi
        QString reg = getString("reg");

        //load target region
        BedFile roi;
        if (reg != "")
        {
            if (QFile::exists(reg))
            {
                roi.load(reg);
            }
            else //parse comma-separated regions
            {
				QStringList regions = reg.split(',');
                foreach(QString region, regions)
                {
                    BedLine line = BedLine::fromString(region);
                    if (!line.isValid()) THROW(ArgumentException, "Invalid region '" + region + "' given in parameter 'reg'!");
                    roi.append(line);
                }
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
        bool sample_one_match = getFlag("sample_one_match");
		QString filter = getString("filter");
		QString id = getString("id");
        QString variant_type = getString("variant_type");
		if (variant_type != "" && !variant_types.contains(variant_type))
		{
			THROW(ArgumentException, "Variant type " + variant_type + " is not a supported variant type!");
		}
		QString info = getString("info");
		QString sample = getString("sample");


        QRegularExpression filter_re;
		if (filter != "")
        {
            // Prepare static filter regexes
			filter_re.setPattern(filter);
            if (!filter_re.isValid())
            {
				THROW(ArgumentException, "Filter regexp '" + filter + "' is not a valid regular expression! ( + " + filter_re.errorString() + " )");
            }
        }

        QRegularExpression id_re;
		if (id != "")
        {
			id_re.setPattern(id);
            if (!id_re.isValid())
            {
				THROW(ArgumentException, "ID regexp '" + id + "' is not a valid regular expression! ( " + id_re.errorString() + " )");
            }
		}

		//parse INFO filters
		QRegExp operator_regex("(\\S+)\\s+(\\S+)\\s+(\\S+)");
		QHash<QByteArray, FilterDefinition> info_filters;
		if (info != "")
		{
			foreach(QString info_filter, info.split(';'))
			{
				info_filter = info_filter.trimmed();

				if (operator_regex.exactMatch(info_filter))
				{
					QStringList matches = operator_regex.capturedTexts();
					QByteArray field = matches[1].toLatin1();
					QByteArray op = matches[2].toLatin1();
					QByteArray value = matches[3].toLatin1();
					FilterDefinition filter_def{op, value};
					if (!filter_def.isValid(op_numeric, op_string)) THROW(ArgumentException, "Invalid filter definition '" + info_filter + "'.");
					info_filters[field] = filter_def;
				}
				else
				{
					THROW(ArgumentException, "Invalid filter definition '" + info_filter + "'");
				}
			}
		}

		//parse sample filters
		QHash<QByteArray, FilterDefinition> sample_filters;
		if (sample != "")
        {
			foreach(QString sample_filter, sample.split(';'))
			{
				sample_filter = sample_filter.trimmed();

				if (operator_regex.exactMatch(sample_filter))
                {
                    QStringList matches = operator_regex.capturedTexts();
					QByteArray field = matches[1].toLatin1();
					QByteArray op = matches[2].toLatin1();
					QByteArray value = matches[3].toLatin1();
					FilterDefinition filter_def{op, value};
					if (!filter_def.isValid(op_numeric, op_string)) THROW(ArgumentException, "Invalid filter definition '" + sample_filter + "'.");
					sample_filters[field] = filter_def;
                }
                else
                {
					THROW(ArgumentException, "Operation '" + sample_filter + "' is invalid!");
                }
            }
        }

        // Read input
		int column_count = 0;
        while (!in_p->atEnd())
        {
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

            if (line.startsWith("#CHROM"))
            {
				column_count = line.count('\t') + 1;
            }
            if (line.startsWith('#'))
            {
                out_p->write(line);
                continue;
            }

            ///Filter by region. First return CHROM and POS and then remove all lines that do not satisfy the check
            if (reg != "")
            {
				QByteArray chr = getPartByColumn(line, VcfFile::CHROM);
				QByteArray start = getPartByColumn(line, VcfFile::POS);
				QByteArray ref = getPartByColumn(line, VcfFile::REF);

				if (roi_index.matchingIndex(chr, start.toInt(), start.toInt() + ref.length())==-1)
                {
                    continue;
                }
            }

            ///Filter by variant_type.
            if (variant_type != "")
			{
				QByteArray ref = getPartByColumn(line, VcfFile::REF).trimmed();
				QByteArray alt = getPartByColumn(line, VcfFile::ALT).trimmed();

                QString type;
                if (ref.length() == 1 && alt.length() == 1)
                {
                    type = "snp";
                }
				else if (alt.contains(','))
				{
					type = "multi-allelic";
				}
				else if (alt.startsWith('<'))
				{
					type = "other";
				}
				else if (ref.length() > 1 || alt.length() > 1)
                {
					type = "indel";
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
				QByteArray qual = getPartByColumn(line, VcfFile::QUAL).trimmed();
				bool ok;
				double qual_value = qual.toDouble(&ok);
				if (!ok && qual!=".") THROW(ProgrammingException, "Quality '" + qual + "' cannot be converted to a number in line: " + line);
				if (qual_value < quality)
				{
                    continue;
                }
            }

            ///Filter by empty filters (will remove empty filters).
            if (filter_empty)
            {
				QByteArray filter = getPartByColumn(line, VcfFile::FILTER).trimmed();

				if (filter!="." && filter!="" && filter!="PASS")
                {
                    continue;
                }
            }

            ///Filter FILTER column via regex
			if (filter != "")
            {
				QByteArray filter = getPartByColumn(line, VcfFile::FILTER);
                auto match = filter_re.match(filter);
                if (!match.hasMatch())
                {
                    continue;
                }
            }

            ///Filter ID column via regex
			if (id != "")
            {
				QByteArray id = getPartByColumn(line, VcfFile::ID);
                auto match = id_re.match(id);
                if (!match.hasMatch())
                {
                    continue;
                }
            }

            ///Filter by type and or info operators in INFO column
			if (info_filters.size()!=0)
            {
				QByteArrayList info_parts = getPartByColumn(line, VcfFile::INFO).trimmed().split(';');

				bool passes_filters = true;
				foreach(const QByteArray& info_part, info_parts)
				{
					int sep_index = info_part.indexOf('=');
					if (sep_index!=-1)
					{
						QByteArray name = info_part.left(sep_index);
						if (info_filters.contains(name))
						{
							if (!satisfiesFilter(info_part.mid(sep_index+1), name, info_filters[name], line))
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
			if (sample_filters.size()!=0)
            {
				QByteArrayList format_ids = getPartByColumn(line, VcfFile::FORMAT).trimmed().split(':');

				QList<QByteArrayList> sample_parts;
				for (int i = VcfFile::MIN_COLS + 1; i < column_count; ++i)
				{
					sample_parts.push_back(getPartByColumn(line, i).split(':'));
                }

                bool passed_filter_once = false;
				bool passes_filters = true;
				for (int i=0; i<format_ids.count(); ++i)
				{
					if (sample_filters.contains(format_ids[i]))
					{
						const FilterDefinition& filter_def = sample_filters[format_ids[i]];

						foreach (const QByteArrayList& sample_part, sample_parts)
						{
							if (!satisfiesFilter(sample_part[i], format_ids[i], filter_def, line))
							{
								passes_filters = false;
                                if (!sample_one_match) break;
                            }
                            else if (sample_one_match)
                            {
                                passed_filter_once = true;
                                break;
                            }
						}
					}

                    if ((!passes_filters && !sample_one_match) || (passed_filter_once && sample_one_match)) break;
				}

                if ((!passed_filter_once && sample_one_match) || (!passes_filters && !sample_one_match))
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
