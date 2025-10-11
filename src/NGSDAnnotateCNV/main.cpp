#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "TSVFileStream.h"

struct PathogenicCnv
{
	int p_class;
	double p_cnv_overlap;
	double cnv_overlap;
	bool annotate;
};

bool cnv_class_rev_sort(const PathogenicCnv& i, const PathogenicCnv& j) {
	if (i.p_class == j.p_class) return i.p_cnv_overlap > j.p_cnv_overlap;
	return i.p_class > j.p_class;
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
		changeLog(2024, 11, 28, "Imporved annotation of overlapping pathogenic CNVs.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);
        QElapsedTimer timer;
		timer.start();

		// prepare SQL query
		SqlQuery sql_query = db.getQuery();
		sql_query.prepare("SELECT rcc.class, cnv.start, cnv.end FROM cnv INNER JOIN report_configuration_cnv rcc ON cnv.id = rcc.cnv_id WHERE rcc.class IN ('4', '5') AND cnv.chr = :0 AND cnv.start <= :1 AND :2 <= cnv.end ");

        out << "annotate TSV file..." << QT_ENDL;

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
			QList<PathogenicCnv> pathogenic_cnvs;

			// parse position:
			Chromosome chr = Chromosome(tsv_line[i_chr]);
			int start = Helper::toInt(tsv_line[i_start], "start");
			int end = Helper::toInt(tsv_line[i_end], "end");
			int cnv_length = end - start;

			// get all overlaping CNVs
			sql_query.bindValue(0, chr.strNormalized(true));
			sql_query.bindValue(1, end);
			sql_query.bindValue(2, start);
			sql_query.exec();
			while(sql_query.next())
			{
				PathogenicCnv p_cnv_stats;
				p_cnv_stats.p_class = sql_query.value(0).toInt();
				int p_start = sql_query.value(1).toInt();
				int p_end = sql_query.value(2).toInt();

				// compute overlaps
				int p_cnv_length = p_end - p_start;
				int intersection = std::min(p_end, end) - std::max(p_start, start);
				p_cnv_stats.p_cnv_overlap = (double) intersection / p_cnv_length;
				p_cnv_stats.cnv_overlap = (double) intersection / cnv_length;

				// check if pathogenic cnv should be annotated
				/// pathogenic CNV fully contained within current CNV
				if (p_start >= start && p_end <= end)
				{
					p_cnv_stats.annotate = true;
				}
				/// at least 30% of the pathogenic CNV overlaps with at least 30% of the current CNV
				else if (p_cnv_stats.p_cnv_overlap >= 0.3 && p_cnv_stats.cnv_overlap >= 0.3)
				{
					p_cnv_stats.annotate = true;
				}
				/// current CNV is fully contained within the pathogenic CNV and represents at least 30% of the pathogenic CNV
				else if ((start >= p_start && end <= p_end) && p_cnv_stats.p_cnv_overlap >= 0.3)
				{
					p_cnv_stats.annotate = true;
				}
				else p_cnv_stats.annotate = false;

				// store cnv_stats
				pathogenic_cnvs.append(p_cnv_stats);

			}

			// sort found cnvs desc
			std::sort(pathogenic_cnvs.begin(), pathogenic_cnvs.end(), cnv_class_rev_sort);

			// convert to QByteArray
			QByteArrayList pathogenic_cnv_entries;
			PathogenicCnv p_cnv;
			foreach(p_cnv, pathogenic_cnvs)
			{
				if (p_cnv.annotate)
				{
					pathogenic_cnv_entries.append(QByteArray::number(p_cnv.p_class) + "/" + QByteArray::number(p_cnv.p_cnv_overlap, 'f', 3));
				}
			}

			// update annotation
			if (i_path_cnvs < 0) tsv_line.append(pathogenic_cnv_entries.join(" "));
			else tsv_line[i_path_cnvs] = pathogenic_cnv_entries.join(" ");

			//add annotated line to buffer
			output_buffer << tsv_line.join("\t");
		}

        out << "Writing output file..." << QT_ENDL;
		// open output file and write annotated CNVs to file
		QSharedPointer<QFile> cnv_output_file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream output_stream(cnv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		cnv_output_file->close();


        out << "annotation complete (runtime: " << Helper::elapsedTime(timer) << ")." << QT_ENDL;

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
