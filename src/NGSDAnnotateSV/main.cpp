#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "BedpeFile.h"

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
		setDescription("Annotates the structural variants in a given BEDPE file with the count of pathogenic SVs of classes 4 and 5 found in the NGSD.");
		addInfile("in", "BEDPE file containing structural variants.", false);
		addOutfile("out", "Output BEDPE file containing annotated structural variants.", false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020, 2, 21, "Initial version.");
		changeLog(2020, 2, 27, "Added temporary db table with same processing system.");
		changeLog(2020, 3, 11, "Updated match computation for INS and BND");
		changeLog(2020, 3, 12, "Bugfix in match computation for INS and BND");
		changeLog(2024, 12, 17, "Refactored to only annotate pathogenic SVs from NGSD");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QTextStream out(stdout);

		// open BEDPE file
		BedpeFile svs;
		svs.load(getInfile("in"));

		// create QByteArrayList to buffer output stream
		QByteArrayList output_buffer;

		// copy comments to output buffer
		output_buffer << svs.headers().join('\n') << "\n";

		// check if file already contains NGSD counts
		QList<QByteArray> header = svs.annotationHeaders();
		QList<QByteArray> additional_columns;

		int i_ngsd_pathogenic = header.indexOf("NGSD_PATHOGENIC_SVS");
		if (i_ngsd_pathogenic < 0)
		{
			// no NGSD column found -> append column at the end
			header.append("NGSD_PATHOGENIC_SVS");
			additional_columns.append("");
			i_ngsd_pathogenic = header.size() - 1;
		}
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" << header.join("\t") << "\n";

		//prepare a query for each SV type to find pathogenic SVs of class 4/5
		QByteArray select_class = "SELECT rc.class FROM `report_configuration_sv` rc, ";

		// query to find all class 4/5 svs for a INS
		QByteArray get_class_insertion = "sv_insertion sv WHERE (rc.class='4' OR rc.class='5') AND rc.sv_insertion_id=sv.id AND sv.chr = :0 AND sv.pos <= :1 AND :2 <= (sv.pos + sv.ci_upper)";
		SqlQuery select_class_sv_insertion = db.getQuery();
		select_class_sv_insertion.prepare(select_class + get_class_insertion);

		// query to find all class 4/5 svs for a BND
		QByteArray get_class_translocation = "sv_translocation sv WHERE (rc.class='4' OR rc.class='5') AND rc.sv_translocation_id=sv.id AND sv.chr1 = :0 AND sv.start1 <= :1 AND :2 <= sv.end1 AND sv.chr2 = :3 AND sv.start2 <= :4 AND :5 <= sv.end2";
		SqlQuery select_class_sv_translocation = db.getQuery();
		select_class_sv_translocation.prepare(select_class + get_class_translocation);

		// iterate over structural variants
		for (int i = 0; i < svs.count(); i++)
		{
			BedpeLine& sv = svs[i];

			QList<QByteArray> sv_annotations = sv.annotations();
			// extend annotation by new columns
			if (additional_columns.size() > 0) sv_annotations.append(additional_columns);

			// ignore SVs on special chromosomes
			if (svs[i].chr1().isNonSpecial() && svs[i].chr2().isNonSpecial())
			{
				int count_class_4 = 0;
				int count_class_5 = 0;

				// annotate
				if(sv.type() == StructuralVariantType::BND)
				{
					//Translocation
					//get matches classifed as class 4/5
					// bind values
					select_class_sv_translocation.bindValue(0, sv.chr1().strNormalized(true));
					select_class_sv_translocation.bindValue(1, sv.end1());
					select_class_sv_translocation.bindValue(2, sv.start1());
					select_class_sv_translocation.bindValue(3, sv.chr2().strNormalized(true));
					select_class_sv_translocation.bindValue(4, sv.end2());
					select_class_sv_translocation.bindValue(5, sv.start2());
					// execute query
					select_class_sv_translocation.exec();
					// parse result
					for (int j = 0; j < select_class_sv_translocation.size(); j++)
					{
						select_class_sv_translocation.next();
						if (select_class_sv_translocation.value(0).toInt() == 4) count_class_4++;
						else count_class_5++;
					}
				}
				else if(sv.type() == StructuralVariantType::INS)
				{
					//Insertion
					//get min and max position
					int min_pos = std::min(sv.start1(), sv.start2());
					int max_pos = std::max(sv.end1(), sv.end2());

					//get matches classifed as class 4/5
					// bind values
					select_class_sv_insertion.bindValue(0, sv.chr1().strNormalized(true));
					select_class_sv_insertion.bindValue(1, max_pos);
					select_class_sv_insertion.bindValue(2, min_pos);
					// execute query
					select_class_sv_insertion.exec();
					// parse result
					for (int j = 0; j < select_class_sv_insertion.size(); j++)
					{
						select_class_sv_insertion.next();
						if (select_class_sv_insertion.value(0).toInt() == 4) count_class_4++;
						else count_class_5++;
					}

				}
				else
				{
					//Del, Dup or Inv

                    // queries to find all class 4/5 svs for a DEL/DUP/INV
                    QByteArray overlap_del_dup_inv = "AND sv.chr = :0 AND sv.start_min <= :1 AND :2 <= sv.start_max AND sv.end_min <= :3 AND :4 <= sv.end_max";

					// select query according to SV type
                    SqlQuery query_class = db.getQuery();
                    if (sv.type() == StructuralVariantType::DEL)
                    {
						query_class.prepare(select_class + "sv_deletion sv WHERE (rc.class='4' OR rc.class='5') AND rc.sv_deletion_id=sv.id " + overlap_del_dup_inv);
                    }
                    else if (sv.type() == StructuralVariantType::DUP)
                    {
						query_class.prepare(select_class + "sv_duplication sv WHERE (rc.class='4' OR rc.class='5') AND rc.sv_duplication_id=sv.id " + overlap_del_dup_inv);
                    }
                    else if (sv.type() == StructuralVariantType::INV)
                    {
						query_class.prepare(select_class + "sv_inversion sv WHERE (rc.class='4' OR rc.class='5') AND rc.sv_inversion_id=sv.id " + overlap_del_dup_inv);
                    }
					else THROW(FileParseException, "Invalid SV type in BEDPE line.");

					//get matches classifed as class 4/5
					// bind values
					query_class.bindValue(0, sv.chr1().strNormalized(true));
					query_class.bindValue(1, sv.end1());
					query_class.bindValue(2, sv.start1());
					query_class.bindValue(3, sv.end2());
					query_class.bindValue(4, sv.start2());
					// execute query
					query_class.exec();
					// parse result
					for (int j = 0; j < query_class.size(); j++)
					{
						query_class.next();
						if (query_class.value(0).toInt() == 4) count_class_4++;
						else count_class_5++;
					}
				}

				// write annotations
				if (count_class_4 != 0 || count_class_5 != 0)
				{
					sv_annotations[i_ngsd_pathogenic] = QByteArray::number(count_class_4) + "x class4 /" + QByteArray::number(count_class_5) + "x class5";
				}
			}

			//write annotation back to BedpeLine
			sv.setAnnotations(sv_annotations);

			// store line in output buffer
			output_buffer << sv.toTsv() << "\n";

		}

        out << "writing annotated SVs to file..." << Qt::endl;

		// open output file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(getOutfile("out"),false,false);
		// write buffer to file
		foreach (const QByteArray& line, output_buffer)
		{
			output_file->write(line);
		}
		output_file->close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
