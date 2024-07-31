#include "ToolBase.h"
#include "Statistics.h"
#include "BasicStatistics.h"
#include <zlib.h>
#include <QFileInfo>
#include <QVector>
#include <QElapsedTimer>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	QByteArray sampleName(QString filename)
	{
		QString output = QFileInfo(filename).fileName();
		if (output.endsWith(".gz", Qt::CaseInsensitive)) output = output.left(output.count()-3);
		if (output.endsWith(".bed", Qt::CaseInsensitive)) output = output.left(output.count()-4);
		if (output.endsWith(".cov", Qt::CaseInsensitive)) output = output.left(output.count()-4);
		return output.toUtf8();
	}

	struct BedLineRepresentation
	{
		QByteArray chr;
		QByteArray start;
		QByteArray end;
		QByteArray cov_score;
	};

	//Function to load a coverage file (.cov, .bed and gzipped possible)
	QList<BedLineRepresentation> parseGzFileBedFile(const QString& filename)
	{
		const int buffer_size = 1048576;
		std::vector<char> buffer(buffer_size);
		QList<BedLineRepresentation> lines;

		//open stream
		FILE* instream = fopen(filename.toUtf8().data(), "rb");
		if (!instream)
		{
			qCritical() << "Could not open file:" << filename << "for reading!";
			return lines;
		}
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (!file)
		{
			qCritical() << "Could not open file:" << filename << "for reading!";
			gzclose(file);
			return lines;
		}
		while(!gzeof(file))
		{
			char* char_array = gzgets(file, buffer.data(), buffer_size);
			//handle errors like truncated GZ file
			if (!char_array)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(file, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					qCritical() << "Error while reading file:" << filename << ":" << error_message;
					break;
				}

				continue;
			}

			QByteArray line(char_array);
			line = line.trimmed();
			if(line.isEmpty()) continue;

			//skip headers
			if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
			{
				continue;
			}

			//error when less than 4 fields
			QByteArrayList fields = line.split('\t');
			if (fields.count()<4)
			{
				THROW(FileParseException, "COV file line with less than three fields found: '" + line + "'");
			}

			//check that start/end is number
			bool ok = true;
			fields[1].toInt(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid starts position found: '" + line + "'");
			fields[2].toInt(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid end position found: '" + line + "'");
			fields[3].toDouble(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid coverage score found: '" + line + "'");

			BedLineRepresentation bed_line;
			bed_line.chr = fields[0];
			bed_line.start = fields[1];
			bed_line.end = fields[2];
			bed_line.cov_score = fields[3];

			lines.append(bed_line);

		}
		gzclose(file);
		return lines;
	}

	//Function to load the coverage profile of a given file (QByteArray determining which lines to include must be provided as well as the number of lines of the main file)
	QHash<QString, QVector<double>> parseGzFileCovProfile(const QString& filename, QByteArray rows_to_use, int main_file_size)
	{
		const int buffer_size = 1048576;
		std::vector<char> buffer(buffer_size);
		QHash<QString, QVector<double>> cov_profile;

		//open stream
		FILE* instream = fopen(filename.toUtf8().data(), "rb");
		if (!instream)
		{
			qCritical() << "Could not open file:" << filename << "for reading!";
			return cov_profile;
		}
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (!file)
		{
			qCritical() << "Could not open file:" << filename << "for reading!";
			fclose(instream);
			return cov_profile;
		}

		int row_count = 0;
		while(!gzeof(file))
		{
			char* char_array = gzgets(file, buffer.data(), buffer_size);
			//handle errors like truncated GZ file
			if (!char_array)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(file, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					qCritical() << "Error while reading file:" << filename << ":" << error_message;
					break;
				}

				continue;
			}

			QByteArray line(char_array);
			line = line.trimmed();
			if(line.isEmpty()) continue;

			//skip headers
			if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
			{
				continue;
			}

			//error when less than 4 fields
			QByteArrayList fields = line.split('\t');
			if (fields.count()<4)
			{
				THROW(FileParseException, "COV file line with less than three fields found: '" + line + "'");
			}

			//check coverage score is a double
			bool ok = true;
			double cov_score = fields[3].toDouble(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid coverage score found: '" + line + "'");

			//create coverage profile
			if (rows_to_use[row_count])
			{
				cov_profile[fields[0]].append(cov_score);
			}
			++row_count;

		}
		gzclose(file);

		if (!(row_count == main_file_size))
		{
			THROW(FileParseException, "Reference sample contains a different number of lines than main sample: '" + filename + "'");
		}
		return cov_profile;
	}


	virtual void setup()
	{
		setDescription("Create a reference cohort for CNV calling from a list of coverage profiles.");
		setExtendedDescription(QStringList() << "This tool creates a reference cohort for CNV calling by analyzing the correlation between your main sample's coverage"
												" profile and a list of reference coverage profiles. You can exclude known polymorphic regions from the calculation by providing them in BED file format"
												" using the -exclude option. To manage run-time and memory requirements, you can limit the number of reference coverage files used in the similarity calculation"
												" with the -max_ref_samples option (default is 600), and set the maximum number of reference coverage files to include in the output with the -cov_max option (default is 150)."
												" The coverage profiles of the samples that correlate best with the main sample are saved in a TSV file. The TSV file typically contains the chromosome, start, and end positions"
												" in the first three columns, with subsequent columns showing the coverage profiles for each selected reference file.");
		addInfile("in", "Coverage profile of main sample in BED/COV format.", false);
		addInfileList("in_ref", "Reference coverage profiles of other sample in BED/COV format (GZ file supported).", false);
		addOutfile("out", "TSV file with coverage profiles of reference samples.", false);
		//optional
		addInfileList("exclude", "Regions in the given BED file(s) are excluded from the coverage calcualtion.", true);
		addInt("max_ref_samples", "Maximum number of reference coverage files to compare during similarity calculation", true, 600);
		addInt("cov_max", "Best n reference coverage files to include in 'out' based on correlation.", true, 150);
		addString("cols", "Comma-separated list of column names used as key for TSV merging", true, "chr,start,end");

		changeLog(2024,  7, 18, "Initial version."); //TODO Kilian
	}

	virtual void main()
	{

		//init
		QElapsedTimer timer;
		int timer_out;
		QTextStream out(stdout);
		QString in = getInfile("in");
		QStringList exclude_files = getInfileList("exclude");
		QStringList in_refs = getInfileList("in_ref");
		int max_ref_samples = getInt("max_ref_samples");
		int cov_max = getInt("cov_max");
		timer.start();

		//Merge exclude files
		BedFile merged_excludes;
		foreach(QString exclude_file, exclude_files)
		{
			BedFile temp;
			temp.load(exclude_file);
			merged_excludes.add(temp);
		}

		merged_excludes.merge();

		timer_out = timer.restart();
		out << "merge excludes: " << timer_out << "ms" << endl;

		//Determine indices to use
		//load main sample
		QList<BedLineRepresentation> main_file = parseGzFileBedFile(in);
		timer_out = timer.restart();
		out << "load main sample: " << timer_out << "ms" << endl;

		//compute ChromosomalIndex from merged excludes
		ChromosomalIndex<BedFile> exclude_idx(merged_excludes);

		//iterate over main sample and determine indices to use
		QByteArray correct_indices;
		for (int i=0; i<main_file.size(); ++i)
		{
			BedLineRepresentation line = main_file[i];
			bool is_valid = true;

			//exclude empty lines and lines where the coverage is 0
			if (line.chr.isEmpty() || line.cov_score.toDouble() == 0.0)
			{
				is_valid = false;
			}
			//exclude lines overlapping with any exclude region
			else if(!(exclude_idx.matchingIndex(line.chr, line.start.toInt(), line.end.toInt())==-1))
			{
				is_valid = false;
			}
			//exclude sex chromosomes
			else
			{
				QString auto_check = line.chr;
				auto_check.remove(0, 3);
				bool ok = true;
				auto_check.toDouble(&ok);
				if (!ok)
				{
					is_valid = false;
				}
			}
			//include the rest
			correct_indices.append(is_valid);
		}

		timer_out = timer.restart();
		out << "compute indices: " << timer_out << "ms" << endl;

		//Create coverage profile for main_file
		QHash<QString, QVector<double>> cov1;
		for (int i = 0; i < correct_indices.size(); ++i)
		{
			if (!correct_indices[i]) continue;

			const auto& line = main_file[i];
			QString chr = line.chr;
			double cov_score = line.cov_score.toDouble();
			cov1[chr].append(cov_score);
		}

		timer_out = timer.restart();
		out << "create coverage profile main: " << timer_out << "ms" << endl;

		//Load other samples and calculate correlation
		QList<QPair<QString, double>> file2corr;
		QHash<QString, QVector<double>> cov2;

		bool test_time = true;
		//iterate over each reference file
		foreach (const QString& ref_file, in_refs)
		{

			//load coverage profile for ref_file
			cov2.clear();
			cov2 = parseGzFileCovProfile(ref_file, correct_indices, main_file.size());

			//calculate correlation between main_sample and current ref_file
			QVector<double> corr;
			for (auto it = cov1.cbegin(); it != cov1.cend(); ++it)
			{
				const QString& key = it.key();
				corr.append(BasicStatistics::correlation(it.value(), cov2.value(key)));
			}

			//sort correlation coefficents for the current ref_file and safe the median
			std::sort(corr.begin(), corr.end());
			file2corr.append(qMakePair(ref_file, BasicStatistics::median(corr)));
			if (file2corr.size() >= max_ref_samples) break;

			if (test_time)
			{
				timer_out = timer.restart();
				out << "load coverage profile first ref sample: " << timer_out << "ms" << endl;
				test_time = false;
			}
		}

		//sort all reference files by descending correlation coefficent
		std::sort(file2corr.begin(), file2corr.end(), [](const QPair<QString, double> &a, const QPair<QString,double> &b)
		{
			return a.second > b.second;
		});

		//write number of compared coverage files to stdout
		out << "compared number of coverage files: " << file2corr.size() << endl;

		//select best n reference files by correlation
		out << "Selected the following files as reference samples based on correlation: " << endl;
		QStringList best_ref_files;
		double mean_correaltion = 0.0;
		int check_max = 0;


		for (int i = 0; i<file2corr.size(); ++i)
		{
			best_ref_files.append(file2corr[i].first);
			mean_correaltion += file2corr[i].second;
			++check_max;
			out << file2corr[i].first << " : " << file2corr[i].second << endl;
			if (check_max == cov_max) break;
		}

		best_ref_files.sort();

		//compute mean correlation and info output to stdout
		mean_correaltion /= best_ref_files.size();
		out << "Files selected: " << check_max << endl;
		out << "Mean correlation to reference samples is: " << mean_correaltion << endl;

		timer_out = timer.restart();
		out << "correlation computation: " << timer_out * 0.00001667 << " min"  << endl;

		//Merge coverage profiles and store them in a tsv file
		QByteArrayList header = getString("cols").toUtf8().split(',');
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(getOutfile("out"), true);

		//init
		QList<QByteArrayList> tsv_line_list;
		tsv_line_list.reserve(main_file.size());
		for (int i = 0; i < main_file.size(); ++i) {
			tsv_line_list.append(QByteArrayList());
		}
		bool first_file = true;

		foreach(QString ref_file, best_ref_files)
		{
			//add the reference sample name to the header
			header.append(sampleName(ref_file));
			//for the first file the chr, start and end for each region is added to each tsv line
			if (first_file)
			{
				for (int i=0; i<main_file.size(); ++i)
				{
					tsv_line_list[i].append(main_file[i].chr);
					tsv_line_list[i].append(main_file[i].start);
					tsv_line_list[i].append(main_file[i].end);
				}

				first_file = false;
			}

			//load the ref_file and append each output line with the coverage score
			const int buffer_size = 1048576;
			std::vector<char> buffer(buffer_size);
			QByteArrayList fields;

			//open stream
			FILE* instream = fopen(ref_file.toUtf8().data(), "rb");
			gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode

			//iterate over each line
			int line_count = 0;
			while(!gzeof(file))
			{
				char* char_array = gzgets(file, buffer.data(), buffer_size);
				fields.clear();

				QByteArray line(char_array);
				line = line.trimmed();

				//skip empty lines
				if(line.isEmpty()) continue;

				//skip headers
				if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser ")) continue;

				//append the current line with the coverage score
				fields = line.split('\t');
				tsv_line_list[line_count].append(fields[3]);
				++line_count;
			}
			gzclose(file);
		}

		//write the header
		outstream->write('#' + header.join('\t') + '\n');
		//write each output line
		foreach(const QByteArrayList &line, tsv_line_list)
		{
			outstream->write(line.join('\t') + '\n');
		}

		outstream->close();

		timer_out = timer.restart();
		out << "merge tsv: " << timer_out * 0.00001667 << " min" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

