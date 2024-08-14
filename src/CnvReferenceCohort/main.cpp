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
		QByteArray depth;
	};

	//Function to load a BED file with coverage data (.cov, .bed and gzipped possible)
	QList<BedLineRepresentation> parseGzFileBedFile(const QString& filename)
	{
		const int buffer_size = 1048576;
		std::vector<char> buffer(buffer_size);
		QList<BedLineRepresentation> lines;

		//open stream
		FILE* instream = fopen(filename.toUtf8().data(), "rb");
		if (!instream)
		{
			THROW(FileAccessException, "Could not open file for reading: '" + filename + "'!");
		}
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (!file)
		{
			THROW(FileAccessException, "Could not open file for reading: '" + filename + "'!");
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
					THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
				}
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

			//append line
			lines.append(BedLineRepresentation{fields[0], fields[1], fields[2], fields[3]});
		}
		gzclose(file);
		return lines;
	}

	//Function to load the coverage profile of a given file (QByteArray determining which lines to include must be provided as well as the number of lines of the main file)
	void parseGzFileCovProfile(QHash<QByteArray, QVector<double>>& cov_profile, const QString& filename, const QBitArray& rows_to_use, int main_file_size, const QList<BedLineRepresentation>& main_file)
	{
		//clear depth profiles - keeps the capacity!!
		foreach(const QByteArray& chr, cov_profile.keys())
		{
			cov_profile[chr].clear();
		}

		const int buffer_size = 1048576;
		char* buffer = new char[buffer_size];

		//open stream
		FILE* instream = fopen(filename.toUtf8().data(), "rb");
		if (!instream)
		{
			THROW(FileAccessException, "Could not open file for reading: '" + filename + "'!");
		}
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (!file)
		{
			THROW(FileAccessException, "Could not open file for reading: '" + filename + "'!");
		}

		int row_count = 0;
		while(!gzeof(file))
		{
			char* char_array = gzgets(file, buffer, buffer_size);

			//handle errors like truncated GZ file
			if (char_array==nullptr)
			{
				int error_no = Z_OK;
				QByteArray error_message = gzerror(file, &error_no);
				if (error_no!=Z_OK && error_no!=Z_STREAM_END)
				{
					THROW(FileParseException, "Error while reading file '" + filename + "': " + error_message);
				}
				continue;
			}

			//determine end of read line
			int i=0;
			while(i<buffer_size && char_array[i]!='\0' && char_array[i]!='\n' && char_array[i]!='\r')
			{
				++i;
			}

			const QByteArray& line = QByteArray::fromRawData(char_array, i);

//			QByteArray line(char_array);
//			line = line.trimmed();
			if(line.trimmed().isEmpty()) continue;

			//skip headers
			if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
			{
				continue;
			}

			QByteArrayList fields;
			QByteArray currentField;

			for (int i = 0; i < line.size(); ++i)
			{
				if (line[i] == '\t')
				{
					fields.append(currentField);
					currentField.clear();
				}
				else
				{
					currentField.append(line[i]);
				}
			}

			// Append the last field (after the last tab character)
			fields.append(currentField);

			//error when less than 4 fields
			//QByteArrayList fields = line.split('\t');
			if (fields.count()<4)
			{
				THROW(FileParseException, "COV file line with less than three fields found: '" + line + "'");
			}

			//check that chr/start/end match the main file
			if (fields[0]!=main_file[row_count].chr || fields[1]!=main_file[row_count].start || fields[2]!=main_file[row_count].end)
			{
				THROW(FileParseException, "Chromosome or positional information does not match the main file: '" + line + "'");
			}

			//create coverage profile
			if (rows_to_use[row_count])
			{
				cov_profile[fields[0]] << Helper::toDouble(fields[3], "coverage value", line);
			}
			++row_count;
		}
		gzclose(file);

		if (!(row_count == main_file_size))
		{
			THROW(FileParseException, "Reference sample contains a different number of lines than main sample: '" + filename + "'");
		}
	}


	virtual void setup()
	{
		setDescription("Create a reference cohort for CNV calling from a list of coverage profiles.");
		setExtendedDescription(QStringList() << "This tool creates a reference cohort for CNV calling by analyzing the correlation between the main sample coverage profile and a list of reference coverage profiles."
											 << "The coverage profiles of the samples that correlate best with the main sample are saved in the output TSV file."
											 << "The TSV file contains the chromosome, start, and end positions in the first three columns, with subsequent columns showing the coverage for each selected reference file.");
		addInfile("in", "Coverage profile of main sample in BED format.", false);
		addInfileList("in_ref", "Reference coverage profiles of other sample in BED format (GZ file supported).", false);
		addOutfile("out", "Output TSV file with coverage profiles of selected reference samples.", false);
		//optional
		addInfileList("exclude", "Regions in the given BED file(s) are excluded from the coverage calcualtion, e.g. copy-number polymorphic regions.", true);
		addInt("cov_max", "Best n reference coverage files to include in 'out' based on correlation.", true, 150);
		addFlag("debug", "Enable debug output.");

		changeLog(2024,  8, 15, "Initial version."); //TODO Kilian
	}

	virtual void main()
	{

		//init
		QTime timer;
		QTextStream out(stdout);
		QString in = getInfile("in");
		QStringList exclude_files = getInfileList("exclude");
		QStringList in_refs = getInfileList("in_ref");
		int cov_max = getInt("cov_max");
		timer.start();
		bool debug = getFlag("debug");

		//Merge exclude files
		BedFile merged_excludes;
		foreach(QString exclude_file, exclude_files)
		{
			BedFile temp;
			temp.load(exclude_file);
			merged_excludes.add(temp);
		}

		merged_excludes.merge();

		timer.restart();
		if (debug) out << "merge excludes: " << Helper::elapsedTime(timer.restart()) << endl;

		//Determine indices to use
		//load main sample
		QList<BedLineRepresentation> main_file = parseGzFileBedFile(in);
		if (debug) out << "load main sample: " << Helper::elapsedTime(timer.restart()) << endl;

		//compute ChromosomalIndex from merged excludes
		ChromosomalIndex<BedFile> exclude_idx(merged_excludes);

		//iterate over main sample and determine indices to use
		QBitArray correct_indices(main_file.size(), false);
		for (int i=0; i<main_file.size(); ++i)
		{
			const BedLineRepresentation& line = main_file[i];
			bool is_valid = true;

			//exclude empty lines and lines where the coverage is 0
			if (line.depth.toDouble() == 0.0)
			{
				is_valid = false;
			}
			//exclude lines overlapping with any exclude region
			else if(exclude_idx.matchingIndex(line.chr, line.start.toInt(), line.end.toInt())!=-1)
			{
				is_valid = false;
			}
			//exclude sex chromosomes
			else if (!Chromosome(line.chr).isAutosome())
			{
				is_valid = false;
			}
			//include the rest
			correct_indices.setBit(i, is_valid);
		}

		if (debug) out << "compute used indices: " << Helper::elapsedTime(timer.restart()) << endl;

		//Create coverage profile for main_file
		QHash<QByteArray, QVector<double>> cov1;
		for (int i = 0; i < correct_indices.size(); ++i)
		{
			if (!correct_indices[i]) continue;

			const BedLineRepresentation& line = main_file[i];
			cov1[line.chr] << line.depth.toDouble();
		}

		if (debug) out << "create coverage profile main: " << Helper::elapsedTime(timer.restart()) << endl;

		//Load other samples and calculate correlation
		QTime corr_timer;
		QList<QPair<QString, double>> file2corr;
		QHash<QByteArray, QVector<double>> cov2;

		//iterate over each reference file
		corr_timer.start();
		foreach (const QString& ref_file, in_refs)
		{
			timer.restart();

			//load coverage profile for ref_file
			parseGzFileCovProfile(cov2, ref_file, correct_indices, main_file.size(), main_file);
			if (debug) out << "loading coverage profile for " << QFileInfo(ref_file).fileName() << ": " << Helper::elapsedTime(timer.restart()) << endl;

			//calculate correlation between main_sample and current ref_file
			QVector<double> corr;
			for (auto it = cov1.cbegin(); it != cov1.cend(); ++it)
			{
				corr.append(BasicStatistics::correlation(it.value(), cov2.value(it.key())));
			}

			//sort correlation coefficents for the current ref_file and safe the median
			std::sort(corr.begin(), corr.end());
			file2corr.append(qMakePair(ref_file, BasicStatistics::median(corr)));
			if (debug) out << "calculating correlation for " << QFileInfo(ref_file).fileName() << ": " << Helper::elapsedTime(timer.restart()) << endl;
		}

		//sort all reference files by descending correlation coefficent
		std::sort(file2corr.begin(), file2corr.end(), [](const QPair<QString, double> &a, const QPair<QString,double> &b)
		{
			return a.second > b.second;
		});

		if (debug) out << "loading all coverage profiles and compute correlation: " << Helper::elapsedTime(corr_timer.restart()) << endl;

		//write number of compared coverage files to stdout
		out << "compared number of coverage files: " << file2corr.size() << endl;

		//select best n reference files by correlation
		out << "Selected the following files as reference samples based on correlation: " << endl;
		QStringList best_ref_files;
		double mean_correaltion = 0.0;
		for (int i = 0; i<file2corr.size(); ++i)
		{
			best_ref_files << file2corr[i].first;
			mean_correaltion += file2corr[i].second;
			out << QFileInfo(file2corr[i].first).fileName() << " : " << file2corr[i].second << endl;
			if (i+1 == cov_max) break;
		}

		best_ref_files.sort();

		//compute mean correlation and info output to stdout
		mean_correaltion /= best_ref_files.size();
		out << "Mean correlation to reference samples is: " << mean_correaltion << endl;

		if (debug) out << "determining best reference samples and calculating mean correlation: " << Helper::elapsedTime(timer.restart()) << endl;

		//Merge coverage profiles and store them in a tsv file
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(getOutfile("out"), true);
		QVector<gzFile> files;

		foreach(QString ref_file, best_ref_files)
		{
			gzFile file = gzopen(ref_file.toUtf8().constData(), "rb");
			if (file)
			{
				files << file;
			}
			else
			{
				THROW(FileAccessException, "Could not open file for reading: '" + ref_file + "'!")
			}
		}

		const int buffer_size = 1048576;
		std::vector<char> buffer(buffer_size);

		bool done = false;
		while (!done)
		{
			done = true;
			for (int i = 0; i < files.size(); ++i)
			{
				if (gzgets(files[i], buffer.data(), buffer_size) != Z_NULL)
				{
					QByteArray line(buffer.data());
					line = line.trimmed();
					QByteArrayList fields;
					fields = line.split('\t');
					if (i == 0)
					{
						outstream->write(fields[0] + '\t' + fields[1] + '\t' + fields[2]);
					}

					outstream->write('\t' + fields[3]);
					 // If a line was read, we're not done yet
					done = false;

					if (i == files.size()-1) outstream->write("\n");
				}
			}
		}

		foreach (gzFile file, files)
		{
			gzclose(file);
		}
		outstream -> close();

		if (debug) out << "writing output: " << Helper::elapsedTime(timer.restart()) << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

