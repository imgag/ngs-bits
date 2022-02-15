#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Log.h"
#include "Settings.h"
#include "VcfFile.h"
#include <QDir>
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

	virtual void setup()
	{
		setDescription("Exports all SVs from the NGSD into BEDPE files.");
		addOutfile("out_folder", "Output folder for the exported BEDPE files.", false, false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 2, 10, "Initial struct.");
	}

	virtual void main()
	{
		//init
		QString output_folder = getOutfile("out_folder");
		bool test = getFlag("test");
		NGSD db(test);
		QTime timer;
		timer.start();
		QTextStream std_out(stdout);

		//debug
		QTime timer_reset;
		QTime timer_get_cs;
		QTime timer_get_var;
		QTime timer_get_sys;
		QTime timer_write_file;
		double debug_time_reset_timer = 0;
		double debug_time_get_cs = 0;
		double debug_time_get_var = 0;
		double debug_time_get_sys = 0;
		double debug_time_write_file = 0;

		//create BEDPE columns for output file
		BedpeFile bedpe_structure;
		bedpe_structure.setAnnotationHeaders(QList<QByteArray>() << "TYPE" << "PROCESSING_SYSTEM" << "ID" << "FORMAT" << "FORMAT_VALUES");

		QList<StructuralVariantType> sv_types = QList<StructuralVariantType>() << StructuralVariantType::DEL << StructuralVariantType::DUP << StructuralVariantType::INS
																			   << StructuralVariantType::INV << StructuralVariantType::BND;
		QStringList chromosomes = db.getEnum("sv_deletion", "chr");
		QMap<int, QByteArray> callset_cache; // Cache to store callset - processing system relation


		//prepare temp tables in NGSD:

		std_out << "Generating temporary tables in NGSD..." << endl;


		//get sample counts
		std_out << "Get sample counts per processing system..." << endl;
		SqlQuery q_sample_counts = db.getQuery();
		q_sample_counts.exec(QByteArray() + "SELECT ps.processing_system_id, COUNT(sc.id) FROM sv_callset sc INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id "
							 + "WHERE ps.quality != 'bad' AND NOT EXISTS "
							 + "(SELECT 1 FROM merged_processed_samples mps WHERE mps.processed_sample_id = sc.processed_sample_id) GROUP BY ps.processing_system_id");
		QMap<QString,int> sample_counts;
		while (q_sample_counts.next())
		{
			sample_counts.insert(db.getProcessingSystemData(q_sample_counts.value(0).toInt()).name_short, q_sample_counts.value(1).toInt());
		}
		std_out << " done. " << Helper::elapsedTime(timer) << endl;


		//get all valid callset ids (are not bad quality and not merged)
		std_out << "Get all valid callset ids..." << endl;

		QSet<int> valid_cs_ids = db.getValuesInt(QByteArray() + "SELECT sc.id FROM sv_callset sc INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id "
													+ "WHERE ps.quality != 'bad' AND NOT EXISTS "
													+ "(SELECT 1 FROM merged_processed_samples mps WHERE mps.processed_sample_id = sc.processed_sample_id)").toSet();

		std_out << " done. " << Helper::elapsedTime(timer) << endl;



		std_out << "NGSD preperation done. " << Helper::elapsedTime(timer) << endl;



		foreach (StructuralVariantType sv_type, sv_types)
		{
			// prepare output
			QString table_name;
			QString filter;
			switch (sv_type)
			{
				case StructuralVariantType::DEL:
					table_name = "sv_deletion";
					filter = " WHERE chr=:0 ORDER BY `start_min`, `start_max`";
					break;
				case StructuralVariantType::DUP:
					table_name = "sv_duplication";
					filter = " WHERE chr=:0 ORDER BY `start_min`, `start_max`";
					break;
				case StructuralVariantType::INS:
					table_name = "sv_insertion";
					filter = " WHERE chr=:0 ORDER BY `pos`, `ci_upper`";
					break;
				case StructuralVariantType::INV:
					table_name = "sv_inversion";
					filter = " WHERE chr=:0 ORDER BY `start_min`, `start_max`";
					break;
				case StructuralVariantType::BND:
					table_name = "sv_translocation";
					filter = " WHERE chr1=:0 ORDER BY `start1`, `end1`";
					break;
				default:
					THROW(ArgumentException, "Invalid SV type!");
					break;
			}
			std_out << "Extract " << StructuralVariantTypeToString(sv_type) << "..." << endl;

			//init output file
			QString file_path = QDir(output_folder).filePath(table_name + ".bedpe");
			QSharedPointer<QFile> out_file = Helper::openFileForWriting(file_path, false, false);
			QTextStream out(out_file.data());


			//write comments to file
			out << "##fileformat=BEDPE\n"
				<< "##fileDate=" << Helper::dateTime("yyyyMMdd") << "\n";

			//store sample count
			foreach (const QString& key, sample_counts.keys())
			{
				out << "##sample_count=(" + key + ", " + QString::number(sample_counts.value(key)) + ")\n";
			}


			//write header
			out << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B" + bedpe_structure.annotationHeaders().join('\t') + "\n";

			//export svs chromosome-wise
			int line_idx = 0;
			foreach (const QString& chr, chromosomes)
			{
				QList<int> ids = db.getValuesInt("SELECT `id` FROM `" + table_name + "`" + filter, chr);
				std_out << QByteArray::number(ids.size()) << " " << StructuralVariantTypeToString(sv_type) << " for " + chr + " to export... " << Helper::elapsedTime(timer) << endl;
				SqlQuery q_callset_id = db.getQuery();
				q_callset_id.prepare("SELECT `sv_callset_id` FROM `" + table_name + "` WHERE id=:0");

				//get SVs and write to file
				foreach (int id, ids)
				{
//					// get callset id
//					timer_reset.restart();
//					timer_get_cs.restart();
//					q_callset_id.bindValue(0, id);
//					q_callset_id.exec();
//					q_callset_id.next();
//					int cs_id = q_callset_id.value(0).toInt();
//					debug_time_get_cs += timer_get_cs.elapsed()/1000.0;
//					debug_time_reset_timer += timer_reset.elapsed()/1000.0;

					//get SV from NGSD
					timer_get_var.restart();
					int cs_id = -1;
					BedpeLine sv = db.structuralVariant(id, sv_type, bedpe_structure, true, &cs_id);
					debug_time_get_var += timer_get_var.elapsed()/1000.0;

					//skip invalid callsets:
					if (!valid_cs_ids.contains(cs_id)) continue;

					// get processing system
					timer_get_sys.restart();
					QByteArray processing_system;
					if(callset_cache.contains(cs_id))
					{
						processing_system = callset_cache.value(cs_id);
					}
					else
					{
						// get processing system
						processing_system = db.getValue("SELECT sys.name_short FROM `sv_callset` sc " + QByteArray() +
														+ "INNER JOIN `processed_sample` ps ON sc.processed_sample_id = ps.id "
														+ "INNER JOIN `processing_system` sys ON ps.processing_system_id = sys.id WHERE sc.id = :0", false, QString::number(cs_id)).toByteArray();

						//store in cache
						callset_cache.insert(cs_id, processing_system);

					}
					debug_time_get_sys += timer_get_sys.elapsed()/1000.0;

					timer_write_file.restart();
					if (sv_type == StructuralVariantType::BND)
					{
						//special handling: store both directions and add SV id
						sv.setAnnotations(QList<QByteArray>() << StructuralVariantTypeToString(sv_type).toUtf8() << processing_system << QByteArray::number(id));

						//store both variants
						QByteArrayList tsv_line = sv.toTsv().split('\t');
						out << tsv_line.join('\t') << "\n";
						tsv_line[0] = sv.chr2().strNormalized(true);
						tsv_line[1] = QByteArray::number(sv.start2());
						tsv_line[2] = QByteArray::number(sv.end2());
						tsv_line[3] = sv.chr1().strNormalized(true);
						tsv_line[4] = QByteArray::number(sv.start1());
						tsv_line[5] = QByteArray::number(sv.end1());
						out << tsv_line.join('\t') << "\n";
						line_idx++;
					}
					else
					{
						sv.setAnnotations(QList<QByteArray>() << StructuralVariantTypeToString(sv_type).toUtf8() << processing_system << "");
						out << sv.toTsv() << "\n";
					}
					debug_time_write_file += timer_write_file.elapsed()/1000.0;
					line_idx++;
					if (line_idx % 100000 == 0)
					{
						std_out << QByteArray::number(line_idx) << " structural variants exported. " << Helper::elapsedTime(timer) << "\n";
						std_out << "\t getting SV took " << QByteArray::number(debug_time_get_var) << "s \n";
						std_out << "\t getting processing system took " << QByteArray::number(debug_time_get_sys) << "s \n";
						std_out << "\t write file took " << QByteArray::number(debug_time_write_file) << "s \n";
					}
				}
			}

			//close file
			out.flush();
			out_file->close();
		}
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
