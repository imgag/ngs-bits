#include "Exceptions.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "VariantList.h"
#include "BedFile.h"
#include "NGSD.h"
#include "Log.h"
#include "Settings.h"
#include "VcfFile.h"
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
		setDescription("Generates a VCF file with all variants and annotations from the NGSD and a BED file containing the gene information of the NGSD.");
		addOutfile("variants", "Output variant list as VCF.", false, true);

		//optional
		addOutfile("genes", "Optional BED file containing the genes and the gene info (only germline).", true, false);
		addInfile("reference", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFloat("max_af", "Maximum allel frequency of exported variants (default: 0.05).",  true, 0.05);
		addInt("gene_offset", "Defines the number of bases by which the region of each gene is extended.", true, 5000);
		addEnum("mode", "Determines the database which is exported.", true, QStringList() << "germline" << "somatic", "germline");
		addFlag("vicc_config_details", "Includes details about VICC interpretation. Works only in somatic mode.");
		addFlag("debug", "Enables debug output (germline only).");

		changeLog(2023,  6, 14, "Added support for 'germline_mosaic' column in 'variant' table.");
		changeLog(2021,  7, 19, "Code and parameter refactoring.");
		changeLog(2021,  7, 19, "Added support for 'germline_het' and 'germline_hom' columns in 'variant' table.");
		changeLog(2019, 12,  6, "Comments are now URL encoded.");
		changeLog(2019,  9, 25, "Added somatic mode.");
		changeLog(2019,  7, 29, "Added BED file for genes.");
		changeLog(2019,  7, 25, "Initial version of this tool.");
	}

	virtual void main()
	{
		//init
		use_test_db_ = getFlag("test");
		debug_ = getFlag("debug");
		vicc_config_details_ = getFlag("vicc_config_details");
		NGSD db(use_test_db_);
		QTextStream out(stdout);
		max_allel_frequency_ = getFloat("max_af");
		gene_offset_ = getInt("gene_offset");
		QString ref_file = getInfile("reference");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		if (gene_offset_ < 0)
		{
			THROW(CommandLineParsingException, "Gene offset has to be a positive value!");
		}

		//annotate
		QString mode = getEnum("mode");
		if(mode=="germline")
		{
			exportingVariantsToVcfGermline(ref_file, getOutfile("variants"), db);
			if (getOutfile("genes") != "")
			{
				exportingGenesToBed(getOutfile("genes"), db);
			}
		}
		else if(mode=="somatic")
		{
			exportingVariantsToVcfSomatic(ref_file, getOutfile("variants"), db);
		}

		out << "Finished!" << endl;
	}

private:
	bool use_test_db_;
	bool debug_;
	bool vicc_config_details_;
	float max_allel_frequency_;
	int gene_offset_;

	//returns a formatted time string (QByteArray) from a given time in milliseconds
	QByteArray getTimeString(double milliseconds)
	{
		//calculate minutes and seconds
		double s = milliseconds/1000.0;
		double m = floor(s/60.0);
		s -= 60.0 * m;
		double h = floor(m/60.0);
		m -= 60.0 * h;

		//create strings
		QByteArray sec = QByteArray::number(s, 'f', 3) + "s";
		QByteArray min = m==0.0 ? "" : QByteArray::number(m, 'f', 0) + "m ";
		QByteArray hours = h==0.0 ? "" : QByteArray::number(h, 'f', 0) + "h ";

		return hours + min + sec;
	}

	//writes the germline variant annotation data from the NGSD to a vcf file
	void exportingVariantsToVcfSomatic(QString reference_file_path, QString vcf_file_path, NGSD& db)
	{
		// init output stream
		QTextStream out(stdout);
		QElapsedTimer timer;
		timer.start();

		QElapsedTimer ref_lookup;
		qint64 ref_lookup_sum = 0;
		QElapsedTimer count_computation;
		qint64 count_computation_sum = 0;
		qint64 create_header = 0;
		QElapsedTimer db_queries;
		qint64 db_query_sum = 0;
		QElapsedTimer vcf_file_writing;
		qint64  vcf_file_writing_sum = 0;


		out << "Exporting somatic variants to VCF file... " << endl;

		// open input reference file
		FastaFileIndex reference_file(reference_file_path);

		// open output vcf file

		// write meta-information lines
		out << "\twriting header...";
		vcf_file_writing.start();
		QSharedPointer<QFile> vcf_file = Helper::openFileForWriting(vcf_file_path, true);
		QTextStream vcf_stream(vcf_file.data());

		vcf_stream << "##fileformat=VCFv4.2\n";
		vcf_stream << "##fileDate=" << QDate::currentDate().toString("yyyyMMdd") << "\n";
		vcf_stream << "##source=NGSDExportAnnotationData " << version() << "\n";
		vcf_stream << "##reference=" << reference_file_path << "\n";
		vcf_file_writing_sum += vcf_file_writing.elapsed();

		// write contigs
		foreach (const QString& chr_name, db.getEnum("variant", "chr"))
		{
			// get chr length
			int chr_length = reference_file.lengthOf(Chromosome(chr_name));

			// write meta information
			vcf_file_writing.restart();
			vcf_stream << "##contig=<ID=" << chr_name << ",length=" << chr_length << ">\n";
			vcf_file_writing_sum += vcf_file_writing.elapsed();
		}

		// define query to get the NGSD counts for each variant
		db_queries.restart();
		SqlQuery ngsd_count_query = db.getQuery();
		ngsd_count_query.prepare("SELECT s.id, dsv.processed_sample_id_tumor, p.name FROM detected_somatic_variant as dsv, variant as v, processed_sample ps, sample as s, project as p WHERE ps.project_id=p.id AND ps.quality!='bad'"
								 " AND dsv.processed_sample_id_tumor=ps.id AND dsv.variant_id=v.id AND ps.sample_id=s.id AND s.tumor='1' AND v.chr=:0 AND v.start=:1 AND v.end=:2 AND v.ref=:3 AND v.obs=:4");
		db_query_sum += db_queries.elapsed();

		// define query to get the variant information by id
		db_queries.restart();
		SqlQuery variant_query = db.getQuery();
		variant_query.prepare("SELECT v.chr, v.start, v.end, v.ref, v.obs, v.id FROM variant as v, detected_somatic_variant as dsv WHERE dsv.id=:0 AND dsv.variant_id=v.id");
		db_query_sum += db_queries.elapsed();

		// write info column descriptions
		vcf_file_writing.restart();
		vcf_stream << "##INFO=<ID=SOM_C,Number=1,Type=Integer,Description=\"Somatic variant count in the NGSD.\">\n";
		vcf_stream << "##INFO=<ID=SOM_P,Number=.,Type=String,Description=\"Project names of project containing this somatic variant in the NGSD.\">\n";
		vcf_stream << "##INFO=<ID=SOM_VICC,Number=1,Type=String,Description=\"Somatic variant interpretation according VICC standard in the NGSD.\">\n";
		vcf_stream << "##INFO=<ID=SOM_VICC_COMMENT,Number=1,Type=String,Description=\"Somatic VICC interpretation comment in the NGSD.\">\n";

		if(vicc_config_details_)
		{
			foreach(const QString& key, SomaticViccData().configAsMap().keys())
			{
				if(key.contains("comment")) continue; //skip comment because it is already included
				vcf_stream << "##INFO=<ID=SOM_VICC_" + key.toUpper() +",Number=1,Type=String,Description=\"Somatic VICC value for VICC parameter " + key + " in the NGSD.\">\n";
			}
		}

		// write header line
		vcf_stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\n";
		vcf_file_writing_sum += vcf_file_writing.elapsed();

		out << "done" << endl;
		create_header = timer.elapsed();

		int variant_count = 0;

		// iterate over database chromosome-wise
		foreach (const QString& chr_name, db.getEnum("variant", "chr"))
		{
			out << "\texporting somatic variants from " << chr_name << "... " << endl;

			// get all ids of all variants on this chromosome
			db_queries.restart();
			QList<int> somatic_variant_ids = db.getValuesInt("SELECT dsv.id FROM detected_somatic_variant as dsv, variant as v WHERE dsv.variant_id=v.id AND chr='" + chr_name + "' ORDER BY v.start ASC, v.end ASC");
			db_query_sum += db_queries.elapsed();

			// cache all processed variant ids to export each variant only once
			QVector<int> processed_variants_ids;

			int variant_count_per_chr = 0;

			// iterate over all variants
			foreach (int som_variant_id, somatic_variant_ids)
			{
				Variant variant;
				db_queries.restart();
				variant_query.bindValue(0, som_variant_id);
				variant_query.exec();
				db_query_sum += db_queries.elapsed();

				// parse query
				if (variant_query.size() == 1)
				{
					variant_query.first();
					variant.setChr(Chromosome(variant_query.value(0).toByteArray()));
					variant.setStart(variant_query.value(1).toInt());
					variant.setEnd(variant_query.value(2).toInt());
					variant.setRef(variant_query.value(3).toByteArray());
					variant.setObs(variant_query.value(4).toByteArray());
					int variant_id = variant_query.value(5).toInt();

					// skip if variant has already been processed
					if (processed_variants_ids.contains(variant_id)) continue;
					variant_count_per_chr++;

					QByteArrayList info_column;

					// compute NGSD count for the somatic variant
					// (adapted from NGSD::annotateSomatic())

					// benchmark
					count_computation.restart();

					ngsd_count_query.bindValue(0, variant.chr().strNormalized(true));
					ngsd_count_query.bindValue(1, variant.start());
					ngsd_count_query.bindValue(2, variant.end());
					ngsd_count_query.bindValue(3, variant.ref());
					ngsd_count_query.bindValue(4, variant.obs());
					ngsd_count_query.exec();

					//process variants
					QMap<QByteArray, int> project_map;
					QSet<QByteArray> processed_ps_ids;
					QSet<QByteArray> processed_s_ids;
					while(ngsd_count_query.next())
					{
						QByteArray current_sample = ngsd_count_query.value(0).toByteArray();
						QByteArray current_ps_id = ngsd_count_query.value(1).toByteArray();
						QByteArray current_project = ngsd_count_query.value(2).toByteArray();

						//skip already seen processed samples
						// (there could be several variants because of indel window,
						//   but we want to process only one)
						if (processed_ps_ids.contains(current_ps_id)) continue;
						processed_ps_ids.insert(current_ps_id);

						//skip already seen samples for general statistics
						// (there could be several processings of the same sample because of
						//   different processing systems or because of experment repeats due to
						//   quality issues)
						if (processed_s_ids.contains(current_sample)) continue;
						processed_s_ids.insert(current_sample);

						// count
						if(!project_map.contains(current_project)) project_map.insert(current_project,0);
						++project_map[current_project];
					}

					// calculate somatic count
					int somatic_count = 0;
					QList<QByteArray> somatic_projects;
					for(auto it=project_map.cbegin(); it!=project_map.cend(); ++it)
					{
						somatic_count += it.value();
						somatic_projects << VcfFile::encodeInfoValue(it.key()).toUtf8();
					}

					// add counts to info column
					if (somatic_count > 0)
					{
						info_column.append("SOM_C=" + QByteArray::number(somatic_count));
						if (somatic_projects.size() > 0)
						{
							info_column.append("SOM_P=" + somatic_projects.join(","));
						}
						else
						{
							info_column.append("SOM_P=.");
						}

					}


					//Add somatic VICC interpretation
					if(db.getSomaticViccId(variant) != -1)
					{
						SomaticViccData data = db.getSomaticViccData(variant);

						info_column.append("SOM_VICC=" + VcfFile::encodeInfoValue(SomaticVariantInterpreter::viccScoreAsString(data)).toUtf8() );
						info_column.append("SOM_VICC_COMMENT=" + VcfFile::encodeInfoValue(data.comment).toUtf8() );

						if(vicc_config_details_)
						{
							QMap<QString, QString> config_details = data.configAsMap();
							for(auto it = config_details.begin() ; it != config_details.end(); ++it)
							{
								info_column.append("SOM_VICC_" + it.key().toUpper().toUtf8() + "=" + VcfFile::encodeInfoValue(it.value()).toUtf8());
							}
						}
					}


					// benchmark
					count_computation_sum += count_computation.elapsed();


					// modify sequence if deletion or insertion occurs (to fit VCF specification)
					if ((variant.ref() == "-") || (variant.obs() == "-"))
					{
						// benchmark
						ref_lookup.restart();

						//include base before (after) to the variant
						QByteArray new_ref_seq, new_obs_seq;
						if (variant.start() != 1)
						{
							// update position for deletion
							if (variant.obs() == "-")
							{
								variant.setStart(variant.start() - 1);
							}

							// add base before ref and alt sequence
							Sequence previous_base = reference_file.seq(variant.chr(),
																		variant.start(), 1);
							new_ref_seq = previous_base + variant.ref();
							new_obs_seq = previous_base + variant.obs();
						}
						else
						{
							// add base after ref and alt sequence
							Sequence next_base = reference_file.seq(variant.chr(),
																	variant.start() + 1, 1);
							new_ref_seq = variant.ref() + next_base;
							new_obs_seq = variant.obs() + next_base;
						}
						new_ref_seq.replace("-", "");
						new_obs_seq.replace("-", "");
						variant.setRef(new_ref_seq);
						variant.setObs(new_obs_seq);

						// benchmark
						ref_lookup_sum += ref_lookup.elapsed();
					}
					vcf_file_writing.restart();
					vcf_stream << variant.chr().strNormalized(true) << "\t";
					vcf_stream << variant.start() << "\t";
					vcf_stream << variant_id << "\t";
					vcf_stream << variant.ref() << "\t";
					vcf_stream << variant.obs() << "\t";
					vcf_stream << "." << "\t"; //quality
					vcf_stream << "." << "\t"; //filter

					// concat all info entries
					if (info_column.size() > 0)
					{
						vcf_stream << info_column.join(";") << "\n";
					}
					else
					{
						vcf_stream << ".\n";
					}
					vcf_file_writing_sum += vcf_file_writing.elapsed();

					// cache processed variant id
					processed_variants_ids.append(variant_id);
				}
				else
				{
					THROW(DatabaseException, "Invalid number of database results found: " + QString::number(variant_query.size()));
				}

				if(variant_count_per_chr % 10000 == 0)
				{
					out << "\t\t\t" << variant_count_per_chr << " variants of " << chr_name
						<< " exported. " << endl;
				}
			}

			out << "\t...done\n\t\t" << somatic_variant_ids.size() << " variants exported.\n"
				<< "\t\t(runtime: " << getTimeString(timer.elapsed()) << ")" << endl;
			variant_count += somatic_variant_ids.size();
		}

		// close vcf file
		vcf_stream.flush();
		vcf_file->close();

		out << " ...finished, " << variant_count << " variants exported.\n"
			<< "\t(overall runtime: " << getTimeString(timer.elapsed()) << ")\n"
			<< "\t(create header: "<< getTimeString(create_header) << ")\n"
			<< "\t(ref sequence lookup: "<< getTimeString(ref_lookup_sum) << ")\n"
			<< "\t(db query sum: "<< getTimeString(db_query_sum) << ")\n"
			<< "\t(vcf file writing: "<< getTimeString(vcf_file_writing_sum) << ")\n"
			<< "\t(count computation: "<< getTimeString(count_computation_sum) << ")\n"<< endl;
	}

	//Function that stores cached variant counts
	void storeCountCache(QTextStream& out, NGSD& db, QHash<int, GenotypeCounts>& count_cache)
	{
		out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Updating variant counts (" << count_cache.count() << " variants)";
		
		QElapsedTimer timer;
		timer.start();
		
		//update counts
		int tries_max = 5;
		int try_nr = 1;
		while (try_nr <= tries_max)
		{
			try
			{
				db.transaction();

				SqlQuery query = db.getQuery();
				query.prepare("UPDATE variant SET germline_het=:0, germline_hom=:1, germline_mosaic=:2 WHERE id=:3");
				for(auto it=count_cache.begin(); it!=count_cache.end(); ++it)
				{
					query.bindValue(0, it.value().het);
					query.bindValue(1, it.value().hom);
					query.bindValue(2, it.value().mosaic);
					query.bindValue(3, it.key());
					query.exec();
				}

				db.commit();
				break;
			}
			catch (Exception& e)
			{
				out << "Note: count update transaction failed in try number " << try_nr << ": " << e.message() << endl;
				db.rollback();
				if (try_nr<tries_max)
				{
					++try_nr;
				}
				else
				{
					throw e;
				}
			}
		}
		
		out << " took " << getTimeString(timer.nsecsElapsed()/1000000.0) << endl;

		//clear cache
		count_cache.clear();
	}

	//writes the somantic variant annotation data from the NGSD to a vcf file
	void exportingVariantsToVcfGermline(QString reference_file_path, QString vcf_file_path, NGSD& db)
	{
		QHash<int, GenotypeCounts> count_cache;

		//init
		QTextStream out(stdout);
		FastaFileIndex reference_file(reference_file_path);
		
		//cache infos from NGSD to avoid joins with detected variant table
		out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Caching sample data... " << endl;
		
		struct ProcessedSampleInfo
		{
			bool bad_quality = false;
			int s_id = -1;
			bool affected = false;
			QString disease_group = "";
		};
		
		QHash<int, ProcessedSampleInfo> ps_infos;
		SqlQuery query = db.getQuery();
		query.exec("SELECT ps.id, ps.quality, s.id, s.disease_status, s.disease_group FROM processed_sample ps, sample s WHERE ps.sample_id=s.id");
		while(query.next())
		{
			int id = query.value(0).toInt();
			ProcessedSampleInfo info;
			info.bad_quality = query.value(1).toString()=="bad";
			info.s_id = query.value(2).toInt();
			info.affected = query.value(3).toString()=="Affected";
			info.disease_group = query.value(4).toString();
			ps_infos.insert(id, info);
		}

		//cache classification data
		out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Caching classification data... " << endl;

		struct ClassificationData
		{
			QByteArray classification = "";
			QByteArray comment = "";
		};

		QHash<int, ClassificationData> class_infos;
		query.exec("SELECT variant_id, class, comment FROM variant_classification");
		while(query.next())
		{
			int variant_id =query.value(0).toInt();
			ClassificationData info;
			info.classification = query.value(1).toByteArray().trimmed().replace("n/a", "");
			info.comment = VcfFile::encodeInfoValue(query.value(2).toByteArray()).toUtf8();
			class_infos.insert(variant_id, info);
		}

		//prepare queries
		SqlQuery ngsd_count_query = db.getQuery();
		ngsd_count_query.prepare("SELECT processed_sample_id, genotype, mosaic FROM detected_variant WHERE variant_id=:0");

		//timers
		QElapsedTimer chr_timer;
		QElapsedTimer tmp_timer;
		double ref_lookup_sum = 0;
		double vcf_file_writing_sum = 0;
		double ngsd_count_query_sum = 0;
		double ngsd_count_calculation_sum = 0;
		double ngsd_count_update = 0;

		out << "Exporting germline variants to VCF file... " << endl;

		// open output vcf file

		// write meta-information lines
		tmp_timer.start();
		QSharedPointer<QFile> vcf_file = Helper::openFileForWriting(vcf_file_path, true);
		QTextStream vcf_stream(vcf_file.data());

		vcf_stream << "##fileformat=VCFv4.2\n";
		vcf_stream << "##fileDate=" << QDate::currentDate().toString("yyyyMMdd") << "\n";
		vcf_stream << "##source=NGSDExportAnnotationData " << version() << "\n";
		vcf_stream << "##reference=" << reference_file_path << "\n";

		// write contigs
		foreach (const QString& chr_name, db.getEnum("variant", "chr"))
		{
			// get chr length
			int chr_length = reference_file.lengthOf(Chromosome(chr_name));

			// write meta information
			vcf_stream << "##contig=<ID=" << chr_name << ",length=" << chr_length << ">\n";
		}

		// get disease groups
		QStringList disease_groups = db.getEnum("sample", "disease_group");

		// write info column descriptions
		vcf_stream << "##INFO=<ID=COUNTS,Number=2,Type=Integer,Description=\"Homozygous/Heterozygous variant counts in NGSD.\">\n";

		// create info column entry for all disease groups
		for(int i = 0; i < disease_groups.size(); i++)
		{
			vcf_stream << "##INFO=<ID=GSC" << QByteArray::number(i + 1).rightJustified(2, '0')
					   << ",Number=2,Type=Integer,Description=\""
					   << "Homozygous/Heterozygous variant counts in NGSD for "
					   << disease_groups[i].toLower() << ".\">\n";
		}
		vcf_stream << "##INFO=<ID=HAF,Number=0,Type=Flag,Description=\"Indicates a allele frequency above a threshold of " << max_allel_frequency_ << ".\">\n";
		vcf_stream << "##INFO=<ID=CLAS,Number=1,Type=String,Description=\"Classification from the NGSD.\">\n";
		vcf_stream << "##INFO=<ID=CLAS_COM,Number=1,Type=String,Description=\"Classification comment from the NGSD.\">\n";
		vcf_stream << "##INFO=<ID=COM,Number=1,Type=String,Description=\"Variant comments from the NGSD.\">\n";

		// write header line
		vcf_stream << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\n";
		vcf_stream.flush();
		vcf_file_writing_sum += tmp_timer.nsecsElapsed()/1000000.0;

		// iterate over database chromosome-wise
		foreach (const QString& chr_name, QStringList() << db.getEnum("variant", "chr"))
		{
			chr_timer.start();

			// get all ids of all variants on this chromosome
			tmp_timer.restart();
			SqlQuery variant_query = db.getQuery();
			variant_query.exec("SELECT chr, start, end, ref, obs, gnomad, comment, germline_het, germline_hom, germline_mosaic, id FROM variant WHERE chr='" + chr_name + "' ORDER BY start ASC, end ASC");
			out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Getting variants for " << chr_name << " took " << getTimeString(tmp_timer.nsecsElapsed()/1000000.0) << endl;

			// iterate over all variants
			while(variant_query.next())
			{
				QElapsedTimer v_timer;
				if (debug_) v_timer.start();

				//parse query
				Variant variant;
				variant.setChr(Chromosome(variant_query.value(0).toByteArray()));
				variant.setStart(variant_query.value(1).toInt());
				variant.setEnd(variant_query.value(2).toInt());
				variant.setRef(variant_query.value(3).toByteArray());
				variant.setObs(variant_query.value(4).toByteArray());
				QByteArray gnomad = variant_query.value(5).toByteArray();
				QByteArray comment = variant_query.value(6).toByteArray();
				int germline_het = variant_query.value(7).toInt();
				int germline_hom = variant_query.value(8).toInt();
				int germline_mosaic = variant_query.value(9).toInt();
				int variant_id = variant_query.value(10).toInt();

				//prepend reference base required in VCF to insertions/deletions
				if ((variant.ref() == "-") || (variant.obs() == "-"))
				{
					//check that coordinates are inside the chromosome
					if (variant.start()>reference_file.lengthOf(variant.chr()))
					{
						out << "Variant " << variant.toString() << " skipped because chromosomal position is after chromosome end!" << endl;
						continue;
					}

					// benchmark
					tmp_timer.restart();

					//include base before (after) to the variant
					QByteArray new_ref_seq, new_obs_seq;
					if (variant.start() != 1)
					{

						// update position for deletion
						if (variant.obs() == "-")
						{
							variant.setStart(variant.start() - 1);
						}

						// add base before ref and alt sequence
						Sequence previous_base = reference_file.seq(variant.chr(), variant.start(), 1);
						new_ref_seq = previous_base + variant.ref();
						new_obs_seq = previous_base + variant.obs();
					}
					else
					{
						// add base after ref and alt sequence
						Sequence next_base = reference_file.seq(variant.chr(), variant.start() + 1, 1);
						new_ref_seq = variant.ref() + next_base;
						new_obs_seq = variant.obs() + next_base;
					}
					new_ref_seq.replace("-", "");
					new_obs_seq.replace("-", "");
					variant.setRef(new_ref_seq);
					variant.setObs(new_obs_seq);

					// benchmark
					ref_lookup_sum += tmp_timer.nsecsElapsed()/1000000.0;
				}
				tmp_timer.restart();
				vcf_stream << variant.chr().strNormalized(true) << "\t";
				vcf_stream << variant.start() << "\t";
				vcf_stream << variant_id << "\t";
				vcf_stream << variant.ref() << "\t";
				vcf_stream << variant.obs() << "\t";
				vcf_stream << "." << "\t"; //quality
				vcf_stream << "." << "\t"; //filter
				vcf_file_writing_sum += tmp_timer.nsecsElapsed()/1000000.0;

				QByteArrayList info_column;

				if(gnomad.toDouble() <= max_allel_frequency_)
				{
					// calculate NGSD counts for each variant
					int count_het = 0;
					int count_hom = 0;
					int count_mosaic = 0;
					//counts per group/status
					QHash<QString, int> hom_per_group, het_per_group;
					QSet<int> samples_done_het, samples_done_hom, samples_done_mosaic;
					tmp_timer.start();
					ngsd_count_query.bindValue(0, variant_id);
					ngsd_count_query.exec();
					ngsd_count_query_sum += tmp_timer.nsecsElapsed()/1000000.0;

					tmp_timer.start();
					while(ngsd_count_query.next())
					{
						int ps_id = ngsd_count_query.value(0).toInt();
						QByteArray genotype = ngsd_count_query.value(1).toByteArray();
						bool mosaic = ngsd_count_query.value(2).toBool();
						
						//ignore processed samples imported while this tool is running
						if (!ps_infos.contains(ps_id)) continue; 
						
						//ignore bad processed samples
						const ProcessedSampleInfo& info = ps_infos[ps_id];
						if (info.bad_quality) continue; 

						//use sample ID to prevent counting variants several times if a
						//sample was sequenced more than once.

						// count heterozygous variants
						if (genotype == "het")
						{
							if (!mosaic && !samples_done_het.contains(info.s_id))
							{
								++count_het;
								samples_done_het << info.s_id;
								samples_done_het.unite(db.sameSamples(info.s_id));

								if (info.affected)
								{
									het_per_group[info.disease_group] += 1;
								}
							}
							if (mosaic && !samples_done_mosaic.contains(info.s_id))
							{
								++count_mosaic;
								samples_done_mosaic << info.s_id;
								samples_done_mosaic.unite(db.sameSamples(info.s_id));
							}
						}

						// count homozygous variants
						if (genotype == "hom" && !samples_done_hom.contains(info.s_id))
						{
							++count_hom;
							samples_done_hom << info.s_id;
							samples_done_hom.unite(db.sameSamples(info.s_id));

							if (info.affected)
							{
								hom_per_group[info.disease_group] += 1;
							}
						}
					}
					ngsd_count_calculation_sum += tmp_timer.nsecsElapsed()/1000000.0;

					// store counts in vcf
					info_column.append("COUNTS=" + QByteArray::number(count_hom) + "," + QByteArray::number(count_het));

					for(int i = 0; i < disease_groups.size(); i++)
					{
						if ((het_per_group.value(disease_groups[i], 0) > 0) || (hom_per_group.value(disease_groups[i], 0) > 0))
						{
							info_column.append("GSC" + QByteArray::number(i + 1).rightJustified(2, '0')
											   + "="
											   + QByteArray::number(hom_per_group.value(disease_groups[i], 0))
											   + ","
											   + QByteArray::number(het_per_group.value(disease_groups[i], 0)));
						}
					}

					// update variant table if counts changed
					if (count_het!=germline_het || count_hom!=germline_hom || count_mosaic!=germline_mosaic)
					{
						count_cache.insert(variant_id, GenotypeCounts{count_het, count_hom, count_mosaic});
						if (count_cache.count()>=10000)
						{
							tmp_timer.restart();
							storeCountCache(out, db, count_cache);
							ngsd_count_update += tmp_timer.nsecsElapsed()/1000000.0;

							//flush VCF stream to disk from time to time. Otherwise monitoring the progress is impossible.
							vcf_stream.flush();
						}
					}
				}
				else
				{
					// mark variants with high allele frequeny
					info_column.append("HAF");
				}

				// get classification
				if (class_infos.contains(variant_id))
				{
					QByteArray classification = class_infos[variant_id].classification;
					if (classification != "") info_column.append("CLAS=" + classification);
					QByteArray clas_comment = class_infos[variant_id].comment;
					if (clas_comment != "") info_column.append("CLAS_COM=\"" + clas_comment + "\"");
				}

				// get comment
				if(comment != "")
				{
					info_column.append("COM=\"" + VcfFile::encodeInfoValue(comment).toUtf8() + "\"");
				}

				// concat all info entries
				tmp_timer.restart();
				if (info_column.size() > 0)
				{
					vcf_stream << info_column.join(";") << "\n";
				}
				else
				{
					vcf_stream << ".\n";
				}
				vcf_file_writing_sum += tmp_timer.nsecsElapsed()/1000000.0;

				if (debug_) out << variant.toString(false) << " gnomAD=" << gnomad << " time=" << getTimeString(v_timer.elapsed()) << endl;
			}

			out << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Finished " << chr_name << ": " << query.size() << " variants exported.\n"
				<< "  " << getTimeString(chr_timer.elapsed()) << " overall\n"
				<< "  " << getTimeString(ref_lookup_sum) << " for ref sequence lookup\n"
				<< "  " << getTimeString(vcf_file_writing_sum) << " for vcf writing\n"
				<< "  " << getTimeString(ngsd_count_query_sum) << " for database queries (variant counts)\n"
				<< "  " << getTimeString(ngsd_count_calculation_sum) << " for genotype calucations (variant counts)\n"
				<< "  " << getTimeString(ngsd_count_update) << " for database update (variant counts)\n";
			out.flush();
		}

		//store remaining entries in cache
		storeCountCache(out, db, count_cache);

		// close vcf file
		vcf_stream.flush();
		vcf_file->close();
	}

/*
 *  writes the gene annotation data from the NGSD to a bed file
 */
	void exportingGenesToBed(QString bed_file_path, NGSD& db)
		{
			// init output stream
			QTextStream out(stdout);
			QElapsedTimer timer;
			timer.start();

			out << "Exporting genes to BED file... " << endl;

			// open BED file
			BedFile output_bed_file;
			int exported_genes = 0;
			int skipped_genes = 0;

			// get gene names:
			GeneSet genes = db.approvedGeneNames();
			QMap<QString, QPair<int,int>> gene_types;
			foreach (const QString& type, db.getEnum("gene", "type"))
			{
				gene_types.insert(type, qMakePair(0,0));
			}

			// iterate over each gene
			foreach (const QByteArray& gene, genes)
			{
				// get additional info
				GeneInfo gene_info = db.geneInfo(gene);

				// calculate region
				GeneSet single_gene;
				single_gene.insert(gene);
				BedFile gene_region = db.genesToRegions(single_gene, Transcript::ENSEMBL, "gene", true,
														false);

				// extend bed file entries and summarize overlapping bed lines:
				gene_region.extend(gene_offset_);
				gene_region.merge();

				// iterate over all entries in the bed file
				for(int i = 0; i < gene_region.count(); i++)
				{
					// generating bed line
					QByteArrayList annotation;
					annotation << gene + " (inh=" + gene_info.inheritance.toUtf8()
								  + " oe_syn=" + gene_info.oe_syn.toUtf8()
								  + " oe_mis="+ gene_info.oe_mis.toUtf8()
								  + " oe_lof=" + gene_info.oe_lof.toUtf8() + ")";
					BedLine bed_line(gene_region[i].chr().strNormalized(true), gene_region[i].start(), gene_region[i].end(), annotation);
					exported_genes++;
					output_bed_file.append(bed_line);
				}

			}

			// sort bed flie and write to file
			output_bed_file.sort();
			output_bed_file.store(bed_file_path);

			out << " ...finished, " << exported_genes << " genes exported, " << skipped_genes << ".\n";
			foreach (QString type, db.getEnum("gene", "type"))
			{
				out << "\t" << type << ": \t" << gene_types[type].first << ", "
					<< gene_types[type].second << "\n";
			}
			out	<<"\n\t (runtime: " << getTimeString(timer.elapsed()) << ") \n" << endl;
		}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
