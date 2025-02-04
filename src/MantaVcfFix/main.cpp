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

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)

	{
	}

	virtual void setup()
	{
		setDescription("Fixes issues in VCF of Manta SV calls.");
		setExtendedDescription(QStringList() << "Removes invalid VCF lines containing empty REF entries."
											 << "Removes duplicate SV calls from Manta VCFs."
							   );
		addInfile("in", "Input VCF file.", false, true);
		addOutfile("out", "Output VCF file.", false, true);

		//optional:
		addFlag("debug", "Print verbose output to STDERR.");


		changeLog(2024,  10, 31, "initial commit.");
		changeLog(2025,   2,  4, "added filtering.");
	}

	virtual void main()
	{
		//static variables
		static const int buffer_size = 1048576; //1MB buffer
		static char* buffer = new char[buffer_size];

		QString in = getInfile("in");
		QString out = getOutfile("out");
		bool debug = getFlag("debug");

		//open output stream
		QSharedPointer<QFile> out_stream = Helper::openFileForWriting(out, true);

		//open input steam
		FILE* file = in.isEmpty() ? stdin : fopen(in.toUtf8().data(), "rb");
		if (file==nullptr) THROW(FileAccessException, "Could not open file '" + in + "' for reading!");
		gzFile in_stream = gzdopen(fileno(file), "rb"); //always open in binary mode because windows and mac open in text mode
		if (in_stream==nullptr) THROW(FileAccessException, "Could not open file '" + in + "' for reading!");

		//cache to store read SVs
		QMap<QByteArray,QByteArrayList> cache;

		//read lines
		while(!gzeof(in_stream))
		{
			char* char_array = gzgets(in_stream, buffer, buffer_size);

			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(in_stream, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading input: " + error_message);
				}
			}

			QByteArray line = QByteArray(char_array);

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//keep header unchanged
			if(line.startsWith("#"))
			{
				out_stream->write(line);
				continue;
			}

			QByteArrayList parts = line.split('\t');
			Helper::trim(parts);

			//remove variants with empty REF entry
			if (parts.at(VcfFile::REF).isEmpty())
			{
				if (debug) qDebug() << "Removed SV with empty REF column at " + parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS);
				continue;
			}

			//remove duplicate SVs (INS) from the manta calls

			//get prefix of MantaID
			QList<QByteArray> manta_id = parts.at(VcfFile::ID).split(':');
			if (manta_id.at(0).startsWith("Manta"))
			{
				manta_id[4] = "X";
			}
			else //DRAGEN VCF
			{
				manta_id[5] = "X";
			}

			QByteArray manta_id_prefix = parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS) + "_" + manta_id.join(':');

			if(cache.contains(manta_id_prefix))
			{
				if (debug) qDebug() << "Skip duplicate variant at " + parts.at(VcfFile::CHROM) + "_" + parts.at(VcfFile::POS);
				//skip SV
				continue;
			}
			else
			{
				cache.insert(manta_id_prefix, parts);
			}

			//write line
			out_stream->write(parts.join("\t") + "\n");

		}

		out_stream->flush();
		out_stream->close();

	}
		/*






























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
		QRegExp operator_regex("(\\S+)\\s+(\\S+)\\s+(\\S+)");
		QList<FilterDefinition> info_filters;
		foreach(QString info_filter, info.split(';'))
		{
			info_filter = info_filter.trimmed();
			if (info_filter.isEmpty()) continue;

			if (operator_regex.exactMatch(info_filter))
			{
				QStringList matches = operator_regex.capturedTexts();
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

			if (operator_regex.exactMatch(sample_filter))
			{
				QStringList matches = operator_regex.capturedTexts();
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
		*/
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
