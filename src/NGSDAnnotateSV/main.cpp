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
		setDescription("Annotates the structural variants of a given BEDPE file by the NGSD counts.");
		addInfile("in", "BEDPE file containing structural variants.", false);
		addOutfile("out", "Output BEDPE file containing annotated structural variants.", false);
		addString("ps", "Processed sample name.", false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("ignore_processing_system", "Use all SVs for annotation (otherwise only SVs from good samples of the same processing system are used)");
		addFlag("debug", "Provide additional information in STDOUT (e.g. query runtime)");
		addFlag("use_memory", "Creates the temporary tables in memory.");

		setExtendedDescription(QStringList() << "NOTICE: the parameter '-ignore_processing_system' will also use SVs from low quality samples (bad samples).");

		changeLog(2020, 2, 21, "Initial version.");
		changeLog(2020, 2, 27, "Added temporary db table with same processing system.");
		changeLog(2020, 3, 11, "Updated match computation for INS and BND");
		changeLog(2020, 3, 12, "Bugfix in match computation for INS and BND");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QByteArray ps_name = getString("ps").toUtf8();
		QTextStream out(stdout);
		QTime timer;
		timer.start();
		bool debug = getFlag("debug");
		bool ignore_processing_system = getFlag("ignore_processing_system");
		QTime init_timer, del_timer, dup_timer, inv_timer, ins_timer, bnd_timer;
		int time_init=0, time_sum_del=0, time_sum_dup=0, time_sum_inv=0, time_sum_ins=0, time_sum_bnd=0;
		int n_del=0, n_dup=0, n_inv=0, n_ins=0, n_bnd=0;
		int sample_count = 0; // number of samples with the same processing system in the NGSD

		QByteArray db_engine;
		if (getFlag("use_memory")) db_engine = "ENGINE=MEMORY "; // create temp dbs in memory


		if (debug) init_timer.start();

		// get processed sample id
		QByteArray sql_exclude_prev_callset;
		QString ps_id = db.processedSampleId(ps_name, false);
		if (ps_id != "")
		{
			out << "Processed sample id: " << ps_id << endl;

			// check if processed sample has already been imported
			QByteArray previous_callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id).toByteArray();

			if(previous_callset_id!="")
			{
				sql_exclude_prev_callset = "sc.id != " + previous_callset_id + " AND ";
				out << "NOTE: Processed sample '" << ps_name << "' already imported. Ignoring SVs of this sample in the annotation." << endl;
			}
		}
		else
		{
			out << "WARNING: Processed sample '" << ps_name << "' not found in NGSD. Processing system cannot be determined! Ignoring processing system for annotation" << endl;
			ignore_processing_system = true;
		}

		// create temporary tables for each SV type filtered by the current processing system
		QByteArray table_prefix;
		if (!ignore_processing_system)
		{
			// get processing system of current sample
			int processing_system_id = db.processingSystemIdFromProcessedSample(ps_name);

			// create a temp table with all valid ps ids ignoring bad/merged samples
			SqlQuery temp_table = db.getQuery();
			temp_table.exec("CREATE TEMPORARY TABLE temp_valid_sv_cs_ids " + db_engine + "SELECT sc.id FROM sv_callset sc INNER JOIN processed_sample ps ON sc.processed_sample_id = ps.id WHERE " + sql_exclude_prev_callset + "ps.processing_system_id = " + QByteArray::number(processing_system_id) + " AND ps.quality != 'bad' AND NOT EXISTS (SELECT 1 FROM merged_processed_samples mps WHERE mps.processed_sample_id = sc.processed_sample_id)");

			// get number of valid callset ids (= known samples)
			sample_count = db.getValue("SELECT COUNT(*) FROM temp_valid_sv_cs_ids").toInt();

			// generate joined tables of all SV types which were called on the same processing system.d
			SqlQuery create_temp_table = db.getQuery();
			QByteArrayList table_names;
			table_names << "sv_deletion" << "sv_duplication" << "sv_inversion";
			foreach (QByteArray table_name, table_names)
			{
				// DEL, DUP, INV
				create_temp_table.exec("CREATE TEMPORARY TABLE temp_" + table_name + " " + db_engine + "SELECT sv.id, sv.sv_callset_id, sv.chr, sv.start_min, sv.start_max, sv.end_min, sv.end_max FROM "
									   + table_name + " sv INNER JOIN temp_valid_sv_cs_ids tt ON sv.sv_callset_id = tt.id");
			}
			//INS
			create_temp_table.exec("CREATE TEMPORARY TABLE temp_sv_insertion " + db_engine + "SELECT sv.id, sv.sv_callset_id, sv.chr, sv.pos, sv.ci_upper FROM sv_insertion sv INNER JOIN temp_valid_sv_cs_ids tt ON sv.sv_callset_id = tt.id");
			//BND
			create_temp_table.exec("CREATE TEMPORARY TABLE temp_sv_translocation " + db_engine + "SELECT sv.id, sv.sv_callset_id, sv.chr1, sv.start1, sv.end1, sv.chr2, sv.start2, sv.end2 FROM sv_translocation sv INNER JOIN temp_valid_sv_cs_ids tt ON sv.sv_callset_id = tt.id");

			// create indices for exact and overlap matching
			SqlQuery create_index = db.getQuery();			
			foreach (QByteArray table_name, table_names)
			{
				// DEL, DUP, INV
				create_index.exec("CREATE INDEX `exact_match` ON temp_" + table_name + "(`chr`, `start_min`, `start_max`, `end_min`, `end_max`)");
				create_index.exec("CREATE INDEX `overlap_match` ON temp_" + table_name + "(`chr`, `start_min`, `end_max`)");
			}
			//INS
			create_index.exec("CREATE INDEX `match` ON temp_sv_insertion(`chr`, `pos`, `ci_upper`)");
			//BND
			create_index.exec("CREATE INDEX `match` ON temp_sv_translocation(`chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`)");

			// set prefix for temp tables
			table_prefix = "temp_";
		}
		else
		{
			// get number of callset ids (= known samples)
			sample_count = db.getValue("SELECT COUNT(*) FROM sv_callset").toInt();
		}

		// prepare a query for each SV
		QByteArray select_count = "SELECT COUNT(*) FROM ";


		QByteArray exact_match_del_dup_inv = "WHERE sv.chr = :0 AND sv.start_min <= :1 AND :2 <= sv.start_max AND sv.end_min <= :3 AND :4 <= sv.end_max";
		QByteArray contained_del_dup_inv = "WHERE sv.chr = :0 AND sv.start_min <= :1 AND :2 <= sv.end_max";

		SqlQuery count_sv_deletion_em = db.getQuery();
		count_sv_deletion_em.prepare(select_count + table_prefix + "sv_deletion sv " + exact_match_del_dup_inv);
		SqlQuery count_sv_duplication_em = db.getQuery();
		count_sv_duplication_em.prepare(select_count + table_prefix + "sv_duplication sv " + exact_match_del_dup_inv);
		SqlQuery count_sv_inversion_em = db.getQuery();
		count_sv_inversion_em.prepare(select_count + table_prefix + "sv_inversion sv " + exact_match_del_dup_inv);

		SqlQuery count_sv_deletion_c = db.getQuery();
		count_sv_deletion_c.prepare(select_count + table_prefix + "sv_deletion sv " + contained_del_dup_inv);
		SqlQuery count_sv_duplication_c = db.getQuery();
		count_sv_duplication_c.prepare(select_count + table_prefix + "sv_duplication sv " + contained_del_dup_inv);
		SqlQuery count_sv_inversion_c = db.getQuery();
		count_sv_inversion_c.prepare(select_count + table_prefix + "sv_inversion sv " + contained_del_dup_inv);

		QByteArray match_ins = table_prefix + "sv_insertion sv WHERE sv.chr = :0 AND sv.pos <= :1 AND :2 <= (sv.pos + sv.ci_upper)";

		SqlQuery count_sv_insertion_m = db.getQuery();
		count_sv_insertion_m.prepare(select_count + match_ins);

		QByteArray match_bnd = table_prefix + "sv_translocation sv WHERE sv.chr1 = :0 AND sv.start1 <= :1 AND :2 <= sv.end1 AND sv.chr2 = :3 AND sv.start2 <= :4 AND :5 <= sv.end2";

		SqlQuery count_sv_translocation_em = db.getQuery();
		count_sv_translocation_em.prepare(select_count + match_bnd);

		if (debug) time_init = init_timer.elapsed();

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
		int i_ngsd_count = header.indexOf("NGSD_COUNT");
		if (i_ngsd_count < 0)
		{
			// no NGSD column found -> append column at the end
			header.append("NGSD_COUNT");
			additional_columns.append("0 (0.0000)");
			i_ngsd_count = header.size() - 1;
		}
		int i_ngsd_count_overlap = header.indexOf("NGSD_COUNT_OVERLAP");
		if (i_ngsd_count_overlap < 0)
		{
			// no NGSD column found -> append column at the end
			header.append("NGSD_COUNT_OVERLAP");
			additional_columns.append("0");
			i_ngsd_count_overlap = header.size() - 1;
		}
		int i_ngsd_pathogenic = header.indexOf("NGSD_PATHOGENIC_SVS");
		if (i_ngsd_pathogenic < 0)
		{
			// no NGSD column found -> append column at the end
			header.append("NGSD_PATHOGENIC_SVS");
			additional_columns.append("0");
			i_ngsd_pathogenic = header.size() - 1;
		}
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" << header.join("\t") << "\n";

		//prepare a query for each SV type to find pathogenic SVs of class 4/5

		QByteArray select_class = "SELECT rc.class FROM `report_configuration_sv` rc, ";

		// queries to find all class 4/5 svs for a DEL/DUP/INV
		QByteArray overlap_del_dup_inv = "AND sv.chr = :0 AND sv.start_min <= :1 AND :2 <= sv.start_max AND sv.end_min <= :3 AND :4 <= sv.end_max";

		SqlQuery select_class_sv_deletion = db.getQuery();
		select_class_sv_deletion.prepare(select_class + "sv_deletion sv WHERE (rc.class='4' || rc.class='5') AND rc.sv_deletion_id=sv.id " + overlap_del_dup_inv);
		SqlQuery select_class_sv_duplication = db.getQuery();
		select_class_sv_duplication.prepare(select_class + "sv_duplication sv WHERE (rc.class='4' || rc.class='5') AND rc.sv_duplication_id=sv.id " + overlap_del_dup_inv);
		SqlQuery select_class_sv_inverison = db.getQuery();
		select_class_sv_inverison.prepare(select_class + "sv_inversion sv WHERE (rc.class='4' || rc.class='5') AND rc.sv_inversion_id=sv.id " + overlap_del_dup_inv);

		// query to find all class 4/5 svs for a INS
		QByteArray get_class_insertion = "sv_insertion sv WHERE (rc.class='4' || rc.class='5') AND rc.sv_insertion_id=sv.id AND sv.chr = :0 AND sv.pos <= :1 AND :2 <= (sv.pos + sv.ci_upper)";
		SqlQuery select_class_sv_insertion = db.getQuery();
		select_class_sv_insertion.prepare(select_class + get_class_insertion);

		// query to find all class 4/5 svs for a BND
		QByteArray get_class_translocation = "sv_translocation sv WHERE (rc.class='4' || rc.class='5') AND rc.sv_translocation_id=sv.id AND sv.chr1 = :0 AND sv.start1 <= :1 AND :2 <= sv.end1 AND sv.chr2 = :3 AND sv.start2 <= :4 AND :5 <= sv.end2";
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
				int ngsd_count_em = 0;
				int ngsd_count_overlap = 0;
				int count_class_4 = 0;
				int count_class_5 = 0;

				// annotate
				if(sv.type() == StructuralVariantType::BND)
				{
					//Translocation
					if (debug) bnd_timer.start();
					//get matches
					// bind values
					count_sv_translocation_em.bindValue(0, sv.chr1().strNormalized(true));
					count_sv_translocation_em.bindValue(1, sv.end1());
					count_sv_translocation_em.bindValue(2, sv.start1());
					count_sv_translocation_em.bindValue(3, sv.chr2().strNormalized(true));
					count_sv_translocation_em.bindValue(4, sv.end2());
					count_sv_translocation_em.bindValue(5, sv.start2());
					// execute query
					count_sv_translocation_em.exec();
					// parse result
					count_sv_translocation_em.next();
					ngsd_count_em = count_sv_translocation_em.value(0).toInt();
					ngsd_count_overlap = count_sv_translocation_em.value(0).toInt();

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

					n_bnd++;
					if (debug) time_sum_bnd += bnd_timer.elapsed();
				}
				else if(sv.type() == StructuralVariantType::INS)
				{
					//Insertion
					if (debug) ins_timer.start();
					//get min and max position
					int min_pos = std::min(sv.start1(), sv.start2());
					int max_pos = std::max(sv.end1(), sv.end2());
					//get exact matches
					// bind values
					count_sv_insertion_m.bindValue(0, sv.chr1().strNormalized(true));
					count_sv_insertion_m.bindValue(1, max_pos);
					count_sv_insertion_m.bindValue(2, min_pos);
					// execute query
					count_sv_insertion_m.exec();
					// parse result
					count_sv_insertion_m.next();
					ngsd_count_em = count_sv_insertion_m.value(0).toInt();
					ngsd_count_overlap = count_sv_insertion_m.value(0).toInt();

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

					n_ins++;
					if (debug) time_sum_ins += ins_timer.elapsed();
				}
				else
				{
					//Del, Dup or Inv

					// get exact matches
					SqlQuery query_em = db.getQuery();
					if (sv.type() == StructuralVariantType::DEL)
					{
						query_em = count_sv_deletion_em;
					}
					else if (sv.type() == StructuralVariantType::DUP) query_em = count_sv_duplication_em;
					else if (sv.type() == StructuralVariantType::INV) query_em = count_sv_inversion_em;
					else THROW(FileParseException, "Invalid SV type in BEDPE line.");

					if (debug)
					{
						if (sv.type() == StructuralVariantType::DEL) del_timer.start();
						else if (sv.type() == StructuralVariantType::DUP) dup_timer.start();
						else inv_timer.start();
					}

					// bind values
					query_em.bindValue(0, sv.chr1().strNormalized(true));
					query_em.bindValue(1, sv.end1());
					query_em.bindValue(2, sv.start1());
					query_em.bindValue(3, sv.end2());
					query_em.bindValue(4, sv.start2());

					// execute query
					query_em.exec();

					// parse result
					query_em.next();
					ngsd_count_em = query_em.value(0).toInt();


					// get contained matches
					SqlQuery query_c = db.getQuery();
					if (sv.type() == StructuralVariantType::DEL) query_c = count_sv_deletion_c;
					else if (sv.type() == StructuralVariantType::DUP) query_c = count_sv_duplication_c;
					else if (sv.type() == StructuralVariantType::INV) query_c = count_sv_inversion_c;
					else THROW(FileParseException, "Invalid SV type in BEDPE line.");

					// bind values
					query_c.bindValue(0, sv.chr1().strNormalized(true));
					query_c.bindValue(1, sv.end2());
					query_c.bindValue(2, sv.start1());

					// execute query
					query_c.exec();

					// parse result
					query_c.next();
					ngsd_count_overlap = query_c.value(0).toInt();

					// select query according to SV type
					SqlQuery query_class = db.getQuery();
					if (sv.type() == StructuralVariantType::DEL) query_class = select_class_sv_deletion;
					else if (sv.type() == StructuralVariantType::DUP) query_class = select_class_sv_duplication;
					else if (sv.type() == StructuralVariantType::INV) query_class = select_class_sv_inverison;
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

					if (debug)
					{
						if (sv.type() == StructuralVariantType::DEL) time_sum_del += del_timer.elapsed();
						else if (sv.type() == StructuralVariantType::DUP) time_sum_dup += dup_timer.elapsed();
						else time_sum_inv += inv_timer.elapsed();
					}

					// count SVs
					if (sv.type() == StructuralVariantType::DEL) n_del++;
					else if (sv.type() == StructuralVariantType::DUP) n_dup++;
					else n_inv++;
				}

				// write annotations
				double af = 0.00;
				if (sample_count != 0) af = (double) ngsd_count_em / (double) sample_count;
				sv_annotations[i_ngsd_count] = QByteArray::number(ngsd_count_em)
						+ " (" + QByteArray::number(af, 'f', 4) + ")";
				sv_annotations[i_ngsd_count_overlap] = QByteArray::number(ngsd_count_overlap);
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

		out << "writing annotated SVs to file..." << endl;

		// open output file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(getOutfile("out"),false,false);
		// write buffer to file
		foreach (const QByteArray& line, output_buffer)
		{
			output_file->write(line);
		}
		output_file->close();

		// debug summary
		if (debug)
		{
			out << "Debug-Output: \n SQL query runtime / SVs:\n";
			out << "\t init: " << (double) time_init/1000.00 << "s \n";
			out << "\t DEL: " << (double) time_sum_del/1000.00 << "s / " << n_del << "\n";
			out << "\t DUP: " << (double) time_sum_dup/1000.00 << "s / " << n_dup << "\n";
			out << "\t INV: " << (double) time_sum_inv/1000.00 << "s / " << n_inv << "\n";
			out << "\t INS: " << (double) time_sum_ins/1000.00 << "s / " << n_ins << "\n";
			out << "\t BND: " << (double) time_sum_bnd/1000.00 << "s / " << n_bnd << "\n";
		}

		out << "finished (" << Helper::elapsedTime(timer) << ") " << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
