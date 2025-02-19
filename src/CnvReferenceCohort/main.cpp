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



	struct BedLineRepresentation
	{
		Chromosome chr;
		int start;
		int end;
		double depth;
		QByteArray chr_start_end;
	};

	struct TabIndices
	{
		int tab1;
		int tab2;
		int tab3;
		int tab4;
	};

	struct MinMaxIndex
	{
		int min;
		int max;
	};

	typedef QVector<double> CoverageProfile;

	QByteArray sampleName(QString filename)
	{
		QString output = QFileInfo(filename).fileName();
		if (output.endsWith(".gz", Qt::CaseInsensitive)) output = output.left(output.count()-3);
		if (output.endsWith(".bed", Qt::CaseInsensitive)) output = output.left(output.count()-4);
		if (output.endsWith(".cov", Qt::CaseInsensitive)) output = output.left(output.count()-4);
		return output.toUtf8();
	}

	TabIndices getTabPosition(const QByteArray& line)
	{
		TabIndices tab_indices;

		tab_indices.tab1 = -1;
		tab_indices.tab2 = -1;
		tab_indices.tab3 = -1;
		tab_indices.tab4 = -1;
		int tab_num = 0;
		for (int i = 0; i < line.size(); ++i)
		{
			if (line[i] == '\t')
			{
				++tab_num;
				if (tab_num==1) tab_indices.tab1 = i;
				else if (tab_num==2) tab_indices.tab2 = i;
				else if (tab_num==3) tab_indices.tab3 = i;
				else if (tab_num==4) tab_indices.tab4 = i;
			}
		}

		if (tab_indices.tab4==-1) tab_indices.tab4 = line.size()-1;

		return tab_indices;
	}

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

			QByteArrayList fields = line.split('\t');

			//error when less then 4 fields
			if (fields.count()<4)
			{
				THROW(FileParseException, "COV file line with less than three fields found: '" + line + "'");
			}

			//check that start/end is number
			bool ok = true;
			int start = fields[1].toInt(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid starts position found: '" + line + "'");
			int end = fields[2].toInt(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid end position found: '" + line + "'");
			double depth = fields[3].toDouble(&ok);
			if (!ok) THROW(FileParseException, "COV file line with invalid coverage score found: '" + line + "'");

			//append line
			lines << BedLineRepresentation{fields[0], start, end, depth, fields[0] + "\t" + fields[1] + "\t" + fields[2]};
		}
		gzclose(file);
		return lines;
	}

	//Function to load the coverage profile of a given file (QByteArray determining which lines to include must be provided as well as the number of lines of the main file)
	void parseGzFileCovProfile(CoverageProfile& cov_profile, const QString& filename, const QBitArray& rows_to_use, int main_file_size, const QList<BedLineRepresentation>& main_file)
	{
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

		int line_index = -1;
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

			QByteArray line(char_array);

			//skip headers
			if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
			{
				continue;
			}

			//skip lines overlapping with excludes
			++line_index;
			if (!rows_to_use[line_index]) continue;

			//determine tab indices
			TabIndices tab_indices = getTabPosition(line);
			

			//error when less than 4 fields
			if (tab_indices.tab3==-1)
			{
				THROW(FileParseException, "COV file line with less than four fields found: '" + line + "'");
			}

			//check chromosomal position is the same
			for (int i=0; i<tab_indices.tab3; ++i)
			{
				if (main_file[line_index].chr_start_end[i]!=line[i])
				{
					THROW(FileParseException, "Chromosomal position '" + line.left(tab_indices.tab3) + "' does not match the main file: '" + main_file[line_index].chr_start_end + "'");
				}
			}

			//set value
			bool ok = false;
			cov_profile[row_count] = line.mid(tab_indices.tab3+1, tab_indices.tab4 - tab_indices.tab3-1).toDouble(&ok);
			++row_count;
			if (!ok) THROW(ArgumentException, "Could not convert depth value " + line.mid(tab_indices.tab3+1, tab_indices.tab4 - tab_indices.tab3-1) + " to double in line: " + line);
		}
		gzclose(file);

		++line_index;
		if (line_index != main_file_size)
		{
			THROW(FileParseException, "Reference sample " + filename + " contains a different number of lines (" + QString::number(line_index) +") than main sample (" + QString::number(main_file_size) +")");
		}
	}


	virtual void setup()
	{
		setDescription("Create a reference cohort for CNV calling from a list of coverage profiles.");
		setExtendedDescription(QStringList() << "This tool creates a reference cohort for CNV calling by analyzing the correlation between the main sample coverage profile and a list of reference coverage profiles."
											 << "The coverage profiles of the samples that correlate best with the main sample are saved in the output TSV file."
											 << "The TSV file contains the chromosome, start, and end positions in the first three columns, with subsequent columns showing the coverage for each selected reference file.");
		addInfile("in", "Coverage profile of main sample in BED format.", false);
		addInfileList("in_ref", "Reference coverage profiles of other sample in BED format (GZ files supported).", false);
		addOutfile("out", "Output TSV file with coverage profiles of selected reference samples.", false);
		//optional
		addInfileList("exclude", "Regions in the given BED file(s) are excluded from the coverage calcualtion, e.g. copy-number polymorphic regions.", true);
		addInt("cov_max", "Best n reference coverage files to include in 'out' based on correlation.", true, 150);
		addFlag("debug", "Enable debug output.");

		changeLog(2024,  8, 16, "Initial version.");
	}

	virtual void main()
	{
		//init
        QElapsedTimer timer;
		QTextStream out(stdout);
		QString in = getInfile("in");
		QStringList exclude_files = getInfileList("exclude");
		QStringList in_refs = getInfileList("in_ref");
		int cov_max = getInt("cov_max");
		bool debug = getFlag("debug");

		//Merge exclude files
		timer.start();
		BedFile merged_excludes;
		foreach(QString exclude_file, exclude_files)
		{
			BedFile temp;
			temp.load(exclude_file);
			merged_excludes.add(temp);
		}
		merged_excludes.merge();
		timer.restart();
        if (debug) out << "merging excludes: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

		//Determine indices to use
		//load main sample and determine row indices for correlation computation
		QList<BedLineRepresentation> main_file = parseGzFileBedFile(in);
        if (debug) out << "loading main sample: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

		//compute ChromosomalIndex from merged excludes
		ChromosomalIndex<BedFile> exclude_idx(merged_excludes);

		//iterate over main sample and determine indices to use
		QBitArray correct_indices(main_file.size(), false);
		for (int i=0; i<main_file.size(); ++i)
		{
			const BedLineRepresentation& line = main_file[i];

			bool is_valid = true;

			//exclude empty lines and lines where the coverage is 0
			if (line.depth == 0.0)
			{
				is_valid = false;
			}
			//exclude lines overlapping with any exclude region
			else if(exclude_idx.matchingIndex(line.chr, line.start, line.end)!=-1)
			{
				is_valid = false;
			}
			//exclude all but autosomes
			else if (!Chromosome(line.chr).isAutosome())
			{
				is_valid = false;
			}
			//include the rest
			correct_indices.setBit(i, is_valid);
		}
        if (debug) out << "computing used indices: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

		QMap<QByteArray, MinMaxIndex> chr_indices;
		int row_count = 0;
		for (int i=0; i < correct_indices.size(); ++i)
		{
			const BedLineRepresentation& line = main_file[i];
			if (correct_indices[i])
			{
				if (chr_indices.contains(line.chr.str()))
				{
					chr_indices[line.chr.str()].max = row_count;
				}
				else
				{
					chr_indices[line.chr.str()].min = row_count;
				}
				++row_count;
			}
		}

        if (debug) out << "calculating min/max indices of chromosomes: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

		//Create coverage profile for main_file
		CoverageProfile cov1;
		for (int i = 0; i < correct_indices.size(); ++i)
		{
			if (!correct_indices[i]) continue;

			const BedLineRepresentation& line = main_file[i];
			cov1 << line.depth;
		}

        if (debug) out << "creating coverage profile of main sample: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

		//Load other samples and calculate correlation
        QElapsedTimer corr_timer;
		QList<QPair<QString, double>> file2corr;
		CoverageProfile cov2(cov1.size());

		//iterate over each reference file
		corr_timer.start();
		foreach (const QString& ref_file, in_refs)
		{
			timer.restart();

			//load coverage profile for ref_file
			parseGzFileCovProfile(cov2, ref_file, correct_indices, main_file.size(), main_file);
            if (debug) out << "loading coverage profile for " << QFileInfo(ref_file).fileName() << ": " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

			//calculate correlation between main_sample and current ref_file
			QVector<double> corr;
			for (auto it = chr_indices.cbegin(); it != chr_indices.cend(); ++it)
			{
				double correlation = BasicStatistics::correlation(cov1, cov2, it.value().min, it.value().max);
				if (BasicStatistics::isValidFloat(correlation))
				{
					corr << correlation;
				}
			}

			//sort correlation coefficents for the current ref_file and safe the median
			std::sort(corr.begin(), corr.end());
			double median_correlation = 0.0;
			if (corr.count()>0) median_correlation = BasicStatistics::median(corr);
			file2corr << qMakePair(ref_file, median_correlation);
            if (debug) out << "calculating correlation for " << QFileInfo(ref_file).fileName() << ": " << Helper::elapsedTime(timer.restart()) << QT_ENDL;
		}

		//sort all reference files by descending correlation coefficent
		std::sort(file2corr.begin(), file2corr.end(), [](const QPair<QString, double> &a, const QPair<QString,double> &b)
		{
			return a.second > b.second;
		});

        if (debug) out << "loading all coverage profiles and compute correlation: " << Helper::elapsedTime(corr_timer.restart()) << QT_ENDL;

		//write number of compared coverage files to stdout
        out << "compared number of coverage files: " << file2corr.size() << QT_ENDL;

		//select best n reference files by correlation + compute mean correlation
        out << "Selected the following files as reference samples based on correlation: " << QT_ENDL;
		QStringList best_ref_files;
		double mean_correaltion = 0.0;
		for (int i = 0; i<file2corr.size(); ++i)
		{
			best_ref_files << file2corr[i].first;
			mean_correaltion += file2corr[i].second;
            out << QFileInfo(file2corr[i].first).fileName() << ": " << file2corr[i].second << QT_ENDL;
			if (best_ref_files.count()>= cov_max) break;
		}
		best_ref_files.sort();
		mean_correaltion /= best_ref_files.size();
        out << "Mean correlation to reference samples is: " << mean_correaltion << QT_ENDL;

        if (debug) out << "determining best reference samples and calculating mean correlation: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;

		//Merge coverage profiles and store them in a tsv file
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(getOutfile("out"), true);
		QVector<gzFile> files;
		files << gzopen(in.toUtf8().constData(), "rb");
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

					TabIndices tab_indices = getTabPosition(line);

					if (i == 0)
					{
						outstream->write(line.left(tab_indices.tab1) + '\t' + line.mid(tab_indices.tab1+1, tab_indices.tab2 - tab_indices.tab1-1) + '\t' + line.mid(tab_indices.tab2+1, tab_indices.tab3 - tab_indices.tab2-1));
					}

					outstream->write('\t' + line.mid(tab_indices.tab3+1, tab_indices.tab4 - tab_indices.tab3-1));
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

        if (debug) out << "writing output: " << Helper::elapsedTime(timer.restart()) << QT_ENDL;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

