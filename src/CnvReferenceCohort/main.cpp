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
		char* buffer = new char[buffer_size];
		QList<QByteArrayList> lines;

		//open stream
		FILE* instream = fopen(filename.toUtf8().data(), "rb");
		if (instream==nullptr)
		{
			qCritical() << "Could not open file:" << filename << "for reading!";
			delete[] buffer;
			return lines;
		}
		gzFile file = gzdopen(fileno(instream), "rb"); //read binary: always open in binary mode because windows and mac open in text mode
		if (file==nullptr)
		{
			qCritical() << "Could not open file:" << filename << "for reading!";
			delete[] buffer;
			return lines;
		}
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
					qCritical() << "Error while reading file:" << filename << ":" << error_message;
					break;
				}

				continue;
			}

			//determine end of read line
			int i=0;
			while(i<buffer_size && char_array[i]!='\0' && char_array[i]!='\n' && char_array[i]!='\r')
			{
				++i;
			}

			QByteArray line = QByteArray::fromRawData(char_array, i).trimmed();
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
				THROW(FileParseException, "BED file line with less than three fields found: '" + line.trimmed() + "'");
			}

			//check that start/end is number
			bool ok = true;
			int start = fields[1].toInt(&ok);
			if (!ok) THROW(FileParseException, "BED file line with invalid starts position found: '" + line.trimmed() + "'");
			int end = fields[2].toInt(&ok);
			if (!ok) THROW(FileParseException, "BED file line with invalid end position found: '" + line.trimmed() + "'");

			fields[1] = QByteArray::number(start);
			fields[2] = QByteArray::number(end);
			lines.append(fields);

		}
		gzclose(file);
		delete[] buffer;

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
		addInfile("in", "Coverage profile of main sample in BED format.", false);
		addInfileList("in_ref", "Reference coverage profiles of other sample.", false);
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
		QElapsedTimer corr_timer;
		QElapsedTimer tsvmerge_timer;
		QString in = getInfile("in");
		QStringList exclude_files = getInfileList("exclude");
		QStringList in_refs = getInfileList("in_ref");
		int max_ref_samples = getInt("max_ref_samples");
		int cov_max = getInt("cov_max");
		corr_timer.start();

		//merge exclude files
		QList<QByteArrayList> merged_excludes;
		foreach(QString exclude_file, exclude_files)
		{
			QList<QByteArrayList> temp;
			temp = parseGzFile(exclude_file);
			merged_excludes.append(temp);
		}

		//determine indices to use
		QList<QByteArrayList> main_file;
		main_file = parseGzFile(in);

		QByteArray correct_indices;
		for (int i=0; i<main_file.count(); ++i)
		{
			QByteArrayList line = main_file[i];
			//skip not valid lines
			if (line.isEmpty()){
				correct_indices.append(false);
				continue;
			}
			else if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
			{
				correct_indices.append(false);
				continue;
			}
			//skip lines with no coverage
			else if(line[3].toDouble()==0.0)
			{
				correct_indices.append(false);
				continue;
			}
			//skip polymorphic regions
			else if (merged_excludes.contains(line))
			{
				correct_indices.append(false);
				continue;

			}

			//skip all chromosomes but autosomes
			QString auto_check = line[0];
			bool ok;
			auto_check.remove(0,3);
			auto_check.toDouble(&ok);

			if (!ok)
			{
				correct_indices.append(false);
				continue;
			}

			correct_indices.append(true);
		}

		//load coverage profile for main_file
		QHash<QString, QVector<double>> cov1; //TODO QHash instead of QMultiMap

		for (int i=0; i<correct_indices.count() ; ++i)
		{
			double cov_score;
			if (!correct_indices[i])
			{
				continue;
			}
			else
			{
				QByteArrayList line = main_file[i];
				QByteArray temp_chr = line[0];
				QByteArray temp_start = line[1];
				QByteArray temp_end = line[2];
				if (line.isEmpty())
				{
					continue;
				}
				else if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
				{
					continue;
				}
				else if (line[3].isNull()){
					THROW(ArgumentException, "Invalid coverage file line in main sample: " + temp_chr + " " + QString::number(temp_start.toInt()) + " " + QString::number(temp_end.toInt()));
				}
				else
				{
					cov_score = line[3].toDouble();
					cov1[temp_chr].append(cov_score);
				}
			}
		}

		//load other samples and calculate correlation
		QMap<double, QString> file2corr;
		QHash<QString, QList<QByteArrayList>> all_ref_files;
		foreach(QString ref_file, in_refs)
		{
			QList<QByteArrayList> file;
			file = parseGzFile(ref_file);
			all_ref_files[ref_file].append(file);

			if (!(file.count()==main_file.count()))
			{
				THROW(ArgumentException, ref_file + " contains a different amount of lines than other reference samples.")
			}

			QHash<QString, QVector<double>> cov2;

			//load coverage profile for ref_file
			for (int i=0; i<correct_indices.count() ; ++i)
			{
				double cov_score;
				if (!correct_indices[i])
				{
					continue;
				}
				else
				{
					QByteArrayList line = file[i];
					QByteArray temp_chr = line[0];
					QByteArray temp_start = line[1];
					QByteArray temp_end = line[2];

					if (line.isEmpty())
					{
						continue;
					}
					else if (line.startsWith("#") || line.startsWith("track ") || line.startsWith("browser "))
					{
						continue;
					}
					else if (line[3].isNull()){
						THROW(ArgumentException, "Invalid coverage file line in file: " + ref_file + " line: " + temp_chr + " " + QString::number(temp_start.toInt()) + " " + QString::number(temp_end.toInt()));
					}
					else
					{
						cov_score = line[3].toDouble();
						cov2[temp_chr].append(cov_score);
					}
				}
			}

			//calculate correlation between main_sample and current ref_file
			QVector<double> corr;
			QHash<QString, QVector<double>>::const_iterator it = cov1.constBegin();
			while (it != cov1.constEnd())
			{
				const QString& key = it.key();
				corr.append(BasicStatistics::correlation(it.value(), cov2.value(key)));
				++it;
			}
			std::sort(corr.begin(), corr.end());
			file2corr.insert(BasicStatistics::median(corr), ref_file);
			if (file2corr.size() >= max_ref_samples) break;

		}
		//write number of compared coverage files to stdout
		qDebug() << "compared number of coverage files:" << file2corr.size();


		//select best n reference files by correlation
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
			if (check_max == cov_max) break;
		}

		best_ref_files.sort();

		//compute mean correlation and info output to stdout
		mean_correaltion = mean_correaltion/best_ref_files.count();
		double elapsed_time = corr_timer.elapsed() * 0.00001667;
		qDebug() << "Time to compute correlation:" << elapsed_time << "minutes";
		qDebug() << "Mean correlation to reference sample is:" << mean_correaltion;
		qDebug() << "Selected the following files as reference samples based on correlation:" << best_ref_files;


		//Merge coverage profiles and store them in a tsv file
		tsvmerge_timer.start();

		QByteArrayList cols = getString("cols").toUtf8().split(',');
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(getOutfile("out"), true);

		QList<QByteArrayList> tsv_file_list;
		bool first_file = true;
		foreach(QString ref_file, best_ref_files)
		{
			cols.append(sampleName(ref_file));
			if (first_file)
			{
				tsv_file_list = all_ref_files[ref_file];
				first_file = false;
			}
			else
			{
				for (int i=0; i<all_ref_files[ref_file].count(); ++i) {
					tsv_file_list[i].append(all_ref_files[ref_file][i][3]);
				}
			}
		}

		outstream->write('#' + cols.join('\t') + '\n');
		foreach(const QByteArrayList &line, tsv_file_list)
		{
			outstream->write(line.join('\t') + '\n');
		}

		elapsed_time = tsvmerge_timer.elapsed() * 0.00001667;
		qDebug() << "Time to merge tsv files:" << elapsed_time << "minutes";

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

