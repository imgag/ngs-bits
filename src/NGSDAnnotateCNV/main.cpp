#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BedpeFile.h"
#include "TSVFileStream.h"

bool cnv_class_rev_sort(QPair<int, double> i, QPair<int, double> j)
{
		if (i.first == j.first) return i.second > j.second;
		return i.first > j.first;
}

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Annotates a CNV file with overlaping pathogenic CNVs from NGSD.");
		addInfile("in", "TSV file containing CNV.", false);
		addOutfile("out", "TSV output file.", false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020, 2, 21, "Initial version.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);
		QTime timer;
		timer.start();

		// prepare SQL query
		SqlQuery sql_query = db.getQuery();
		sql_query.prepare("SELECT rcc.class, cnv.start, cnv.end FROM cnv INNER JOIN report_configuration_cnv rcc ON cnv.id = rcc.cnv_id WHERE rcc.class IN ('4', '5') AND cnv.chr = :0 AND cnv.start <= :1 AND :2 <= cnv.end ");

		out << "annotate TSV file..." << endl;

		// copy comments
		TSVFileStream cnv_input_file(getInfile("in"));
		QByteArrayList output_buffer;
		output_buffer.append(cnv_input_file.comments());

		// check if column already present
		QByteArrayList header = cnv_input_file.header();
		int i_path_cnvs = header.indexOf("ngsd_pathogenic_cnvs");

		// modify header if neccessary
		if (i_path_cnvs < 0) header.append("ngsd_pathogenic_cnvs");

		output_buffer << "#" + header.join("\t");

		// get indices for position
		int i_chr = cnv_input_file.colIndex("chr", true);
		int i_start = cnv_input_file.colIndex("start", true);
		int i_end = cnv_input_file.colIndex("end", true);

		// iterate over input file and annotate each cnv
		while (!cnv_input_file.atEnd())
		{
			// read next line:
			QByteArrayList tsv_line = cnv_input_file.readLine();
			QList<QPair<int,double>> pathogenic_cnvs;

			// parse position:
			Chromosome chr = Chromosome(tsv_line[i_chr]);
			int start = Helper::toInt(tsv_line[i_start], "start");
			int end = Helper::toInt(tsv_line[i_end], "end");

			// get all overlaping CNVs
			sql_query.bindValue(0, chr.strNormalized(true));
			sql_query.bindValue(1, end);
			sql_query.bindValue(2, start);
			sql_query.exec();
			while(sql_query.next())
			{
				int p_class = sql_query.value(0).toInt();
				int p_start = sql_query.value(1).toInt();
				int p_end = sql_query.value(2).toInt();

				// compute overlap
				int p_cnv_length = p_end - p_start;
				int intersection = std::min(p_end, end) - std::max(p_start, start);
				double overlap = (double) intersection / p_cnv_length;

				// store tuple
				pathogenic_cnvs.append(QPair<int, double>(p_class, overlap));

			}

			// sort found cnvs desc
			std::sort(pathogenic_cnvs.begin(), pathogenic_cnvs.end(), cnv_class_rev_sort);

			// convert to QByteArray
			QByteArrayList pathogenic_cnv_entries;
			QPair<int,double> p_cnv;
			foreach(p_cnv, pathogenic_cnvs)
			{
				if (p_cnv.second >= 0.1)
				{
					pathogenic_cnv_entries.append(QByteArray::number(p_cnv.first) + "/" + QByteArray::number(p_cnv.second, 'f', 4));
				}
			}


			// update annotation
			if (i_path_cnvs < 0) tsv_line.append(pathogenic_cnv_entries.join(" "));
			else tsv_line[i_path_cnvs] = pathogenic_cnv_entries.join(" ");

			//add annotated line to buffer
			output_buffer << tsv_line.join("\t");
		}

		out << "Writing output file..." << endl;
		// open output file and write annotated CNVs to file
		QSharedPointer<QFile> cnv_output_file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream output_stream(cnv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		cnv_output_file->close();


		out << "annotation complete (runtime: " << Helper::elapsedTime(timer) << ")." << endl;

	}	
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
