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
	QByteArray field;
	QByteArray op;
	QByteArray value;

	FilterDefinition(const QString& f, const QString& o, const QString& v)
		: field(f.toUtf8())
		, op(o.toUtf8())
		, value(v.toUtf8())
	{
	}

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

	// Checks if a filter is satisified
	static bool satisfiesFilter(const QByteArray& value, const FilterDefinition& filter_def, const QByteArray& line)
	{
		if (filter_def.op == ">")
		{
			return toDouble(value, filter_def.field, line) > filter_def.value.toDouble();
		}
		else if (filter_def.op == ">=")
		{
			return toDouble(value, filter_def.field, line) >= filter_def.value.toDouble();
		}
		else if (filter_def.op == "!=")
		{
			return toDouble(value, filter_def.field, line) != filter_def.value.toDouble();
		}
		else if (filter_def.op == "=")
		{
			return toDouble(value, filter_def.field, line) == filter_def.value.toDouble();
		}
		else if (filter_def.op == "<=")
		{
			return toDouble(value, filter_def.field, line) <= filter_def.value.toDouble();
		}
		else if (filter_def.op == "<")
		{
			return toDouble(value, filter_def.field, line) < filter_def.value.toDouble();
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

	static const QByteArray& col(const QByteArrayList& parts, int index)
	{
		if (index>=parts.count())
		{
			THROW(FileParseException, "Invalid column index " + QString::number(index) + ". The line has " + QString::number(parts.count()) + " tab-separated elements:\n" + parts.join("\t"));
		}
		return parts[index];
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
		setExtendedDescription(QStringList() << "Missing annotation in the SAMPLE filter are treated as passing the filter."
											 << "INFO flags (i.e. entries without value) are ignored, i.e. they cannot be filtered."
							   );
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		addString("reg", "Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.", true);
		addFlag("remove_invalid", "Removes invalid variants, i.e. invalid position of ref/alt.");
		addFlag("remove_non_ref", "Remove '<NON_REF>' entries (used in gVCF files).");
		addString("variant_type", "Filters by variant type. Possible types are: '" + variant_types.join("','") + "'.", true);
		addString("id", "Filter by ID column (regular expression).", true);
		addFloat("qual", "Filter by QUAL column (minimum).", true, 0.0);
		addString("filter", "Filter by FILTER column - keep matches (regular expression).", true);
		addString("filter_exclude", "Filter by FILTER column - exclude matches (regular expression).", true);
		addFlag("filter_clear", "Remove filter entries of all variants, i.e. sets filter to PASS.");
		addFlag("filter_empty", "Removes entries with non-empty FILTER column.");
		addString("info", "Filter by INFO column entries - use ';' as separator for several filters, e.g. 'DP > 5;AO > 2' (spaces are important).\nValid operations are '" + op_numeric.join("','") + "','" + op_string.join("','") + "'.", true);
		addString("sample", "Filter by sample-specific entries - use ';' as separator for several filters, e.g. 'GT is 1/1' (spaces are important).\nValid operations are '" + op_numeric.join("','") + "','" + op_string.join("','") + "'.", true);
		addFlag("sample_one_match", "If set, a line will pass if one sample passes all filters (default behaviour is that all samples have to pass all filters).");
		addFlag("no_special_chr", "Removes variants that are on special chromosomes, i.e. not on autosomes, not on gonosomes and not on chrMT.");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);

		changeLog(2024,  7, 11, "Added flag 'filter_clear'.");
		changeLog(2023, 11, 21, "Added flag 'no_special_chr'.");
		changeLog(2018, 10, 31, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QString reg = getString("reg");

		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

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
		bool remove_invalid = getFlag("remove_invalid");
		bool sample_one_match = getFlag("sample_one_match");
		bool no_special_chr = getFlag("no_special_chr");
		bool remove_non_ref = getFlag("remove_non_ref");
		bool filter_clear = getFlag("filter_clear");
		QString filter = getString("filter");
		QString filter_exclude = getString("filter_exclude");
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

		QRegularExpression filter_exclude_re;
		if (filter_exclude != "")
		{
			// Prepare static filter regexes
			filter_exclude_re.setPattern(filter_exclude);
			if (!filter_exclude_re.isValid())
			{
				THROW(ArgumentException, "Filter regexp '" + filter_exclude + "' is not a valid regular expression! ( + " + filter_exclude_re.errorString() + " )");
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
        QRegularExpression operator_regex("(\\S+)\\s+(\\S+)\\s+(\\S+)");
		QList<FilterDefinition> info_filters;
		foreach(QString info_filter, info.split(';'))
		{
			info_filter = info_filter.trimmed();
			if (info_filter.isEmpty()) continue;

            QRegularExpressionMatch operator_regex_match = operator_regex.match(info_filter);
            if (operator_regex_match.hasMatch())
			{
                QStringList matches = operator_regex_match.capturedTexts();
				FilterDefinition filter(matches[1], matches[2], matches[3]);
				if (!filter.isValid(op_numeric, op_string)) THROW(ArgumentException, "Invalid filter definition '" + info_filter + "'.");
				info_filters << filter;
			}
			else
			{
				THROW(ArgumentException, "Invalid filter definition '" + info_filter + "'");
			}
		}

		//parse sample filters
		QList<FilterDefinition> sample_filters;
		foreach(QString sample_filter, sample.split(';'))
		{
			sample_filter = sample_filter.trimmed();
			if (sample_filter.isEmpty()) continue;

            QRegularExpressionMatch operator_regex_match = operator_regex.match(sample_filter);
            if (operator_regex_match.hasMatch())
			{
                QStringList matches = operator_regex_match.capturedTexts();
				FilterDefinition filter(matches[1], matches[2], matches[3]);
				if (!filter.isValid(op_numeric, op_string)) THROW(ArgumentException, "Invalid filter definition '" + sample_filter + "'.");
				sample_filters << filter;
			}
			else
			{
				THROW(ArgumentException, "Operation '" + sample_filter + "' is invalid!");
			}
		}

		// Read input
		QTextStream std_err(stderr);
		int column_count = 0;
		while (!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//split and trim
			QByteArrayList parts = line.split('\t');
			Helper::trim(parts);

			//handle header columns
			if (line.startsWith('#'))
			{
				if (!line.startsWith("##"))
				{
					column_count = parts.count();
				}

				if (filter_clear && line.startsWith("##FILTER=")) continue;

				out_p->write(line);
				continue;
			}

			//Filter by region
			if (reg != "")
			{
				const QByteArray& chr = col(parts, VcfFile::CHROM);
				const QByteArray& start = col(parts, VcfFile::POS);
				const QByteArray& ref = col(parts, VcfFile::REF);
				int pos = Helper::toInt(start, "genomic position");
				if (roi_index.matchingIndex(chr, pos, pos + ref.length()-1)==-1)
				{
					continue;
				}
			}

			//filter out special chromosomes
			if (no_special_chr && !Chromosome(col(parts, VcfFile::CHROM)).isNonSpecial())
			{
				continue;
			}

			//Filter by variant_type
			if (variant_type != "")
			{
				const QByteArray& ref = col(parts, VcfFile::REF);
				const QByteArray& alt = col(parts, VcfFile::ALT);

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

			//filter out invalid lines
			if (remove_invalid)
			{
				QList<Sequence> alts;
				foreach(const QByteArray& alt, col(parts, VcfFile::ALT).split(',')) alts << alt;
				VcfLine vcf_line(col(parts, VcfFile::CHROM), Helper::toInt(col(parts, VcfFile::POS), "genomic position"), col(parts, VcfFile::REF), alts);
				if (!vcf_line.isValid(reference))
				{
					std_err << "filtered invalid variant: " << vcf_line.chr().strNormalized(true) << ":" << vcf_line.start() << " " << vcf_line.ref() << ">" << vcf_line.altString() << "\n";
					continue;
				}
			}

			//filter out <NON_REF> entries
			if (remove_non_ref)
			{
				QList<Sequence> alts;
				foreach(const QByteArray& alt, col(parts, VcfFile::ALT).split(',')) alts << alt;
				VcfLine vcf_line(col(parts, VcfFile::CHROM), Helper::toInt(col(parts, VcfFile::POS), "genomic position"), col(parts, VcfFile::REF), alts);
				if (alts.contains("<NON_REF>"))
				{
					std_err << "filtered '<NON_REF>' variant: " << vcf_line.chr().strNormalized(true) << ":" << vcf_line.start() << " " << vcf_line.ref() << ">" << vcf_line.altString() << "\n";
					continue;
				}
			}

			//filter by QUALITY
			if (quality>0)
			{
				if (Helper::toDouble(col(parts, VcfFile::QUAL), "quality") < quality)
				{
					continue;
				}
			}

			//filter by empty filters (will remove empty filters).
			if (filter_empty)
			{
				const QByteArray& filter = col(parts, VcfFile::FILTER);

				if (filter!="." && filter!="" && filter!="PASS")
				{
					continue;
				}
			}

			//filter FILTER column via regex (include match)
			if (!filter.isEmpty())
			{
				const QByteArray& filter = col(parts, VcfFile::FILTER);
				auto match = filter_re.match(filter);
				if (!match.hasMatch())
				{
					continue;
				}
			}

			//filter FILTER column via regex (exclude match)
			if (!filter_exclude.isEmpty())
			{
				const QByteArray& filter = col(parts, VcfFile::FILTER);
				auto match = filter_exclude_re.match(filter);
				if (match.hasMatch())
				{
					continue;
				}
			}

			//filter ID column via regex
			if (!id.isEmpty())
			{
				const QByteArray& id = col(parts, VcfFile::ID);
				auto match = id_re.match(id);
				if (!match.hasMatch())
				{
					continue;
				}
			}

			//filter by info operators in INFO column
			if (!info_filters.isEmpty())
			{
				QByteArrayList info_parts = col(parts, VcfFile::INFO).split(';');

				bool passes_filters = true;
				foreach(const QByteArray& info_part, info_parts)
				{
					int sep_index = info_part.indexOf('=');
					if (sep_index==-1) continue; //skip flags without value

					QByteArray name = info_part.left(sep_index);
					foreach(const FilterDefinition& filter, info_filters)
					{
						if (filter.field==name)
						{
							if (!satisfiesFilter(info_part.mid(sep_index+1), filter, line))
							{
								passes_filters = false;
							}
						}
					}
					if (!passes_filters) break;
				}

				if (!passes_filters)
				{
					continue;
				}
			}

			//filter by sample operators in the SAMPLE column
			if (!sample_filters.isEmpty())
			{
				QByteArrayList format_entries = col(parts, VcfFile::FORMAT).split(':');

				int samples_passing = 0;
				int samples_failing = 0;
				for (int i = VcfFile::MIN_COLS + 1; i < column_count; ++i)
				{
					QByteArrayList sample_parts = col(parts, i).split(':');

					bool current_sample_passes = true;
					foreach(const FilterDefinition& filter, sample_filters)
					{
						int index = format_entries.indexOf(filter.field);
						if (index==-1) continue;

						if (!satisfiesFilter(sample_parts[index], filter, line))
						{
							current_sample_passes = false;
							break;
						}
					}
					if(current_sample_passes)
					{
						++samples_passing;
						if (sample_one_match) break;
					}
					else
					{
						++samples_failing;
						if (!sample_one_match) break;
					}
				}

				if ((sample_one_match && samples_passing==0) || (!sample_one_match && samples_failing!=0)) continue;
			}

			//clear filter entries
			if (filter_clear)
			{
				parts[VcfFile::FILTER] = "PASS";
				line = parts.join('\t');
				line.append('\n');
			}

			out_p->write(line);

		}

		//close streams
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
