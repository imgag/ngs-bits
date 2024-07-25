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

	QList<QByteArrayList> parseGzFile(const QString& filename)
	{
		const int buffer_size = 1048576; //1MB buffer
		std::vector<char> buffer(buffer_size);
		QList<QByteArrayList> lines;

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
			fclose(instream);
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

			//error when less than 3 fields
			QByteArrayList fields = line.split('\t');
			if (fields.count()<3)
			{
				THROW(FileParseException, "BED file line with less than three fields found: '" + line + "'");
			}

			//check that start/end is number
			bool ok = true;
			int start = fields[1].toInt(&ok);
			if (!ok) THROW(FileParseException, "BED file line with invalid starts position found: '" + line + "'");
			int end = fields[2].toInt(&ok);
			if (!ok) THROW(FileParseException, "BED file line with invalid end position found: '" + line + "'");

			fields[1] = QByteArray::number(start);
			fields[2] = QByteArray::number(end);
			lines.append(fields);

		}
		gzclose(file);
		//fclose(instream);
		return lines;
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
		addInfile("in", "Coverage profile of main sample in BED/COV format (GZ file supported).", false);
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
		QElapsedTimer corr_timer, tsvmerge_timer;
		QTextStream out(stdout);
		QString in = getInfile("in");
		QStringList exclude_files = getInfileList("exclude");
		QStringList in_refs = getInfileList("in_ref");
		int max_ref_samples = getInt("max_ref_samples");
		int cov_max = getInt("cov_max");

		corr_timer.start();

		//merge exclude files
		BedFile merged_excludes;
		foreach(QString exclude_file, exclude_files)
		{
			BedFile temp;
			temp.load(exclude_file);
			merged_excludes.add(temp);
		}
		merged_excludes.merge();

		qDebug() << "exclude_files merged";

		//determine indices to use
		ChromosomalIndex<BedFile> exclude_idx(merged_excludes);
		QList<QByteArrayList> main_file = parseGzFile(in);

		QByteArray correct_indices;
		correct_indices.reserve(main_file.size());
		for (int i=0; i<main_file.count(); ++i)
		{
			QByteArrayList line = main_file[i];
			bool is_valid = true;

			if (line.isEmpty() || line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser ") ||
				line[3].toDouble() == 0.0 || !(exclude_idx.matchingIndex(line[0], line[1].toInt(), line[2].toInt())==-1))
			{
				is_valid = false;
			}
			else
			{
				QString auto_check = line[0];
				auto_check.remove(0, 3);
				bool ok;
				auto_check.toDouble(&ok);
				if (!ok)
				{
					is_valid = false;
				}
			}

			correct_indices.append(is_valid);
		}

		qDebug() << "correct indices calculated";
		qDebug() << correct_indices;

		//load coverage profile for main_file
		QHash<QString, QVector<double>> cov1;

		for (int i = 0; i < correct_indices.size(); ++i)
		{
			if (!correct_indices[i]) continue;

			const auto& line = main_file[i];
			QString chr = line[0];
			double cov_score = line[3].toDouble();
			cov1[chr].append(cov_score);
		}

		qDebug() << "main file loaded";

		//load other samples and calculate correlation
		QMap<double, QString> file2corr;
		QHash<QString, QVector<QByteArray>> all_ref_files;
		foreach (QString ref_file, in_refs)
		{
			QList<QByteArrayList> file = parseGzFile(ref_file);

			if (file.size() != main_file.size())
			{
				THROW(ArgumentException, ref_file + " contains a different amount of lines than other reference samples.")
			}

			//load coverage profile for ref_file
			QHash<QString, QVector<double>> cov2;
			for (int i = 0; i < correct_indices.size(); ++i)
			{
				all_ref_files[ref_file].append(file[i][3]);
				if (!correct_indices[i]) continue;

				const auto& line = file[i];
				QString chr = line[0];
				double cov_score = line[3].toDouble();
				cov2[chr].append(cov_score);
			}

			//calculate correlation between main_sample and current ref_file
			QVector<double> corr;
			for (auto it = cov1.cbegin(); it != cov1.cend(); ++it)
			{
				const QString& key = it.key();
				corr.append(BasicStatistics::correlation(it.value(), cov2.value(key)));
			}

			std::sort(corr.begin(), corr.end());
			file2corr.insert(BasicStatistics::median(corr), ref_file);
			qDebug() << "reference file " << ref_file << " loaded and correlation computed";
			if (file2corr.size() >= max_ref_samples) break;
		}

		//write number of compared coverage files to stdout
		out << "compared number of coverage files: " << file2corr.size() << endl;


		//select best n reference files by correlation
		out << "Selected the following files as reference samples based on correlation: " << endl;
		QStringList best_ref_files;
		QMapIterator<double, QString> it(file2corr);
		double mean_correaltion = 0.0;
		int check_max = 0;

		it.toBack();
		while (it.hasPrevious())
		{
			it.previous();
			best_ref_files.append(it.value());
			mean_correaltion += it.key();
			++check_max;
			out << it.value() << " : " << it.key() << endl;
			if (check_max == cov_max) break;
		}

		best_ref_files.sort();

		qDebug() << "best ref_files selected";

		//compute mean correlation and info output to stdout
		mean_correaltion /= best_ref_files.size();
		double elapsed_time = corr_timer.elapsed() * 0.00001667;
		out << "Time to compute correlation: " << elapsed_time << " minutes" << endl;
		out << "Mean correlation to reference samples is: " << mean_correaltion << endl;

		//merge coverage profiles and store them in a tsv file
		tsvmerge_timer.start();

		QByteArrayList cols = getString("cols").toUtf8().split(',');
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(getOutfile("out"), true);

		QList<QByteArrayList> tsv_line_list;
		tsv_line_list.reserve(main_file.size());
		for (int i = 0; i < main_file.size(); ++i) {
			tsv_line_list.append(QByteArrayList());
		}
		bool first_file = true;

		foreach(QString ref_file, best_ref_files)
		{
			cols.append(sampleName(ref_file));
			if (first_file)
			{
				for (int i=0; i<main_file.size(); ++i)
				{
					tsv_line_list[i].append(main_file[i][0]);
					tsv_line_list[i].append(main_file[i][1]);
					tsv_line_list[i].append(main_file[i][2]);
				}

				first_file = false;
			}

			for (int i=0; i<all_ref_files[ref_file].size(); ++i)
			{
				tsv_line_list[i].append(all_ref_files[ref_file][i]);
			}
		}

		outstream->write('#' + cols.join('\t') + '\n');
		foreach(const QByteArrayList &line, tsv_line_list)
		{
			outstream->write(line.join('\t') + '\n');
		}

		elapsed_time = tsvmerge_timer.elapsed() * 0.00001667;
		out << "Time to merge tsv files: " << elapsed_time << " minutes" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

