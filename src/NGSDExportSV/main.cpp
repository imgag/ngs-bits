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
		addInt("common_sys_threshold", "Minimal number of samples for which a seperate density file is created.", true, 50);

		changeLog(2022, 2, 10, "Initial struct.");
		changeLog(2022, 2, 18, "Implemented tool.");
		changeLog(2022, 2, 24, "Changed SV break point output format.");
		changeLog(2024, 2,  7, "Added output of processing specific breakpoint density.");
		changeLog(2025, 1, 13, "Added DISEASE_GROUP column to BEDPE files");
	}

	void collapseSvDensity(QString output_folder, QHash<Chromosome, QMap<int,int>> sv_density, const QStringList& chromosomes, const QByteArray& sys="")
	{

		QTextStream std_out(stdout);
		QTime timer_collapse_density;
		double debug_time_collapse_density = 0;
		timer_collapse_density.start();
		BedFile sv_density_file;
		sv_density_file.appendHeader("#track graphtype=bar autoScale=on windowingFunction=none coords=0 name=\"SV break point density" + ((sys.isEmpty())?"":(" (" + sys + ")")) + "\"");

		//iterate over all chromosomes
		foreach (const QString& chr_str, chromosomes)
		{
			Chromosome chr = Chromosome(chr_str);
			int start = -1;
			int end = -1;
			int density_value = 0;
			const QMap<int,int>& current_density = sv_density[chr];


			//iterate over QMap
			foreach (int pos, current_density.keys())
			{
				if(start < 0)
				{
					// start new entry
					start = pos;
					end = pos;
					density_value = current_density.value(pos);
				}
				else
				{
					if ((pos == (end + 1)) && (density_value == current_density.value(pos)))
					{
						//same segment with constant value --> extend BED region
						end = pos;
					}
					else
					{
						//either density value changed or new segment --> finish old segment and start new entry
						sv_density_file.append(BedLine(chr, start + 1, end + 1, QByteArrayList() << "." << QByteArray::number(density_value))); // +1 because BEDPE is 0-based and method requires 1-based positions
						start = pos;
						end = pos;
						density_value = current_density.value(pos);
					}
				}
			}
		}
		debug_time_collapse_density += timer_collapse_density.elapsed();

		//write to file
		if (sys.isEmpty())
		{
			sv_density_file.store(QDir(output_folder).filePath("sv_breakpoint_density.igv"), false);
		}
		else
		{
			sv_density_file.store(QDir(output_folder).filePath("sv_breakpoint_density_" + sys + ".igv"), false);
		}


        std_out << "Collapsing SV density took " << QByteArray::number(debug_time_collapse_density/1000.0) << "s" << QT_ENDL;
	}

	virtual void main()
	{
		//init
		QString output_folder = getOutfile("out_folder");
		bool test = getFlag("test");
		int common_sys_threshold = getInt("common_sys_threshold");
		NGSD db(test);
        QElapsedTimer timer;
		timer.start();
		QTextStream std_out(stdout);

		//debug
		QTime timer_get_var;
		QTime timer_get_sys;
		QTime timer_write_file;
		QTime timer_extract_density;
		double debug_time_get_var = 0;
		double debug_time_get_sys = 0;
		double debug_time_write_file = 0;
		double debug_time_extract_density = 0;


		//create BEDPE columns for output file
		BedpeFile bedpe_structure;
		bedpe_structure.setAnnotationHeaders(QList<QByteArray>() << "TYPE" << "PROCESSING_SYSTEM" << "ID" << "FORMAT" << "FORMAT_VALUES" << "DISEASE_GROUP");
		int idx_type = bedpe_structure.annotationIndexByName("TYPE");
		int idx_processing_system = bedpe_structure.annotationIndexByName("PROCESSING_SYSTEM");
		int idx_sv_id = bedpe_structure.annotationIndexByName("ID");
		int idx_format = bedpe_structure.annotationIndexByName("FORMAT");
		int idx_disease_group = bedpe_structure.annotationIndexByName("DISEASE_GROUP");

		QList<StructuralVariantType> sv_types = QList<StructuralVariantType>() << StructuralVariantType::DEL << StructuralVariantType::DUP << StructuralVariantType::INS
																			   << StructuralVariantType::INV << StructuralVariantType::BND;
		QStringList chromosomes = db.getEnum("sv_deletion", "chr");
		QMap<int, QByteArray> callset_cache; // Cache to store callset - processing system relation


		//init QMap for SV density map
		QHash<Chromosome, QMap<int,int>> sv_density;
		QMap<QByteArray, QHash<Chromosome, QMap<int,int>>> sv_density_per_sys;
		foreach (const QString& chr, chromosomes)
		{
			sv_density.insert(Chromosome(chr), QMap<int,int>());
		}


		//get sample counts
        std_out << "Get sample counts per processing system..." << QT_ENDL;
		SqlQuery q_sample_counts = db.getQuery();
		q_sample_counts.exec(QByteArray() + "SELECT ps.processing_system_id, COUNT(sc.id) FROM sv_callset sc INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id "
							 + "WHERE ps.quality != 'bad' AND NOT EXISTS "
							 + "(SELECT 1 FROM merged_processed_samples mps WHERE mps.processed_sample_id = sc.processed_sample_id) GROUP BY ps.processing_system_id");
		QMap<QString,int> sample_counts;
		while (q_sample_counts.next())
		{
			sample_counts.insert(db.getProcessingSystemData(q_sample_counts.value(0).toInt()).name_short, q_sample_counts.value(1).toInt());
		}
        std_out << " done. " << Helper::elapsedTime(timer) << QT_ENDL;


		//get all common processing systems (will be written in seperate files)
		foreach (const QString& key, sample_counts.keys())
		{
			if (sample_counts.value(key) >= common_sys_threshold) sv_density_per_sys.insert(key.toUtf8(), sv_density);
		}


		//get all valid callset ids (are not bad quality and not merged)
        std_out << "Get all valid callset ids..." << QT_ENDL;

		QSet<int> valid_cs_ids = db.getValuesInt(QByteArray() + "SELECT sc.id FROM sv_callset sc INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id "
													+ "WHERE ps.quality != 'bad' AND NOT EXISTS "
													+ "(SELECT 1 FROM merged_processed_samples mps WHERE mps.processed_sample_id = sc.processed_sample_id)").toSet();

        std_out << " done. " << Helper::elapsedTime(timer) << QT_ENDL;

        std_out << "NGSD preperation done. " << Helper::elapsedTime(timer) << QT_ENDL;


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
            std_out << "Extract " << StructuralVariantTypeToString(sv_type) << "..." << QT_ENDL;

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

			//store disease_groups
			QStringList disease_groups = db.getEnum("sample", "disease_group");
			for(int i = 0; i < disease_groups.size(); i++)
			{
				out << "##INFO=<ID=GSC" << QByteArray::number(i + 1).rightJustified(2, '0') << ",Number=1,Type=String,Description=\"" << "Disease group: " << disease_groups[i].toLower() << ".\">\n";
			}

			//write header
			out << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + bedpe_structure.annotationHeaders().join('\t') + "\n";

			//export svs chromosome-wise
			int line_idx = 0;
			int skipped_svs = 0;
			foreach (const QString& chr, chromosomes)
			{
				QList<int> ids = db.getValuesInt("SELECT `id` FROM `" + table_name + "`" + filter, chr);
                std_out << QByteArray::number(ids.size()) << " " << StructuralVariantTypeToString(sv_type) << " for " + chr + " to export... " << Helper::elapsedTime(timer) << QT_ENDL;
				SqlQuery q_callset_id = db.getQuery();
				q_callset_id.prepare("SELECT `sv_callset_id` FROM `" + table_name + "` WHERE id=:0");

				//get SVs and write to file
				foreach (int id, ids)
				{

					//get SV from NGSD
					timer_get_var.restart();
					int cs_id = -1;
					BedpeLine sv;
					try
					{
						sv = db.structuralVariant(id, sv_type, bedpe_structure, true, &cs_id);
					}
					catch (DatabaseException e)
					{
						std_out << e.message();
						skipped_svs++;
						continue;
					}

					int allele_count = (sv.annotations().at(idx_format).split(':').at(0) == "1/1")?2:1;
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

					//get disease group
					QByteArray disease_group;
					disease_group = db.getValue("SELECT s.disease_group FROM `sv_callset` sc " + QByteArray() +
												+ "INNER JOIN `processed_sample` ps ON sc.processed_sample_id = ps.id "
												+ "INNER JOIN `sample` s ON ps.sample_id = s.id WHERE sc.id = :0", false, QString::number(cs_id)).toByteArray();

					//write to file
					timer_write_file.restart();
					//update annotation
					QList<QByteArray> sv_annotation = sv.annotations();
					sv_annotation[idx_type] = StructuralVariantTypeToString(sv_type).toUtf8();
					sv_annotation[idx_processing_system] = processing_system;
					sv_annotation[idx_disease_group] = "GSC" + QByteArray::number(disease_groups.indexOf(disease_group) + 1).rightJustified(2, '0');
					if (sv_type == StructuralVariantType::BND)
					{
						//special handling: store both directions and add SV id
						sv_annotation[idx_sv_id] = QByteArray::number(id);
						sv.setAnnotations(sv_annotation);

						//1st direction
						QByteArrayList tsv_line = sv.toTsv().split('\t');
						out << tsv_line.join('\t') << "\n";

						//2nd direction
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
						sv.setAnnotations(sv_annotation);
						out << sv.toTsv() << "\n";
					}
					debug_time_write_file += timer_write_file.elapsed()/1000.0;
					line_idx++;


					//calculate SV breakpoint density
					timer_extract_density.restart();
					// 1st breakpoint
					for (int i = sv.start1(); i <= sv.end1(); ++i)
					{
						if (sv_density[sv.chr1()].contains(i))
						{
							sv_density[sv.chr1()][i] += allele_count;
						}
						else
						{
							sv_density[sv.chr1()].insert(i, allele_count);
						}
					}
					//per processing system
					if (sv_density_per_sys.contains(processing_system))
					{
						for (int i = sv.start1(); i <= sv.end1(); ++i)
						{
							if (sv_density_per_sys[processing_system][sv.chr1()].contains(i))
							{
								sv_density_per_sys[processing_system][sv.chr1()][i] += allele_count;
							}
							else
							{
								sv_density_per_sys[processing_system][sv.chr1()].insert(i, allele_count);
							}
						}
					}

					// 2nd breakpoint (except INS)
					if (sv_type != StructuralVariantType::INS)
					{
						for (int i = sv.start2(); i <= sv.end2(); ++i)
						{
							if (sv_density[sv.chr2()].contains(i))
							{
								sv_density[sv.chr2()][i] += allele_count;
							}
							else
							{
								sv_density[sv.chr2()].insert(i, allele_count);
							}
						}
						//per processing system
						if (sv_density_per_sys.contains(processing_system))
						{
							for (int i = sv.start2(); i <= sv.end2(); ++i)
							{
								if (sv_density_per_sys[processing_system][sv.chr2()].contains(i))
								{
									sv_density_per_sys[processing_system][sv.chr2()][i] += allele_count;
								}
								else
								{
									sv_density_per_sys[processing_system][sv.chr2()].insert(i, allele_count);
								}
							}
						}
					}
					debug_time_extract_density += timer_extract_density.elapsed()/1000.0;

					//progress output
					if (line_idx % 100000 == 0)
					{
						std_out << QByteArray::number(line_idx) << " structural variants exported. " << Helper::elapsedTime(timer) << "\n";
						std_out << QByteArray::number(skipped_svs) << " structural variants skipped. \n";
						std_out << "\t getting SV took " << QByteArray::number(debug_time_get_var) << "s \n";
						std_out << "\t getting processing system took " << QByteArray::number(debug_time_get_sys) << "s \n";
						std_out << "\t write file took " << QByteArray::number(debug_time_write_file) << "s \n";
						std_out << "\t extracting SV density took " << QByteArray::number(debug_time_extract_density) << "s \n";
                        std_out << QT_ENDL;
					}
				}

			}

			//close file
			out.flush();
			out_file->close();
		}

		//collape SV density to BED file
		collapseSvDensity(output_folder, sv_density, chromosomes);
		foreach (const QByteArray& sys, sv_density_per_sys.keys())
		{
			collapseSvDensity(output_folder, sv_density_per_sys[sys], chromosomes, sys);
		}

	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
