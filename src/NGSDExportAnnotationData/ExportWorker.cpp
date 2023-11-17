#include "ExportWorker.h"
#include "Exceptions.h"
#include <QElapsedTimer>

ExportWorker::ExportWorker(QString chr, const ExportParameters& params, const SharedData& shared_data)
	: QObject()
	, QRunnable()
	, chr_(chr)
	, params_(params)
	, shared_data_(shared_data)
{
	if (params_.verbose) QTextStream(stdout) << "ExportWorker" << endl;
}

ExportWorker::~ExportWorker()
{
	if (params_.verbose) emit log(chr_, "~ExportWorker");
}

void ExportWorker::run()
{
	try
	{
		//init
		FastaFileIndex reference_file(params_.ref_file);
		NGSD db(params_.use_test_db);

		//export germline
		if (!params_.germline.isEmpty())
		{
			QString tmp_vcf = params_.tempVcf(chr_, "germline");
			emit log(chr_, "Starting germline export to " + tmp_vcf);

			QHash<int, GenotypeCounts> count_cache;

			QElapsedTimer chr_timer;
			chr_timer.start();
			QElapsedTimer tmp_timer;
			double ref_lookup_sum = 0;
			double vcf_file_writing_sum = 0;
			double ngsd_count_query_sum = 0;
			double ngsd_count_calculation_sum = 0;
			double ngsd_count_update = 0;
			long long vcf_lines_written = 0;

			//prepare queries
			SqlQuery ngsd_count_query = db.getQuery();
			ngsd_count_query.prepare("SELECT processed_sample_id, genotype, mosaic FROM detected_variant WHERE variant_id=:0");

			// write meta-information lines
			QSharedPointer<QFile> vcf_file = Helper::openFileForWriting(tmp_vcf, true);
			QTextStream vcf_stream(vcf_file.data());

			// get all ids of all variants on this chromosome
			tmp_timer.restart();
			SqlQuery variant_query = db.getQuery();
			variant_query.exec("SELECT chr, start, end, ref, obs, gnomad, comment, germline_het, germline_hom, germline_mosaic, id FROM variant WHERE chr='" + chr_ + "' ORDER BY start ASC, end ASC");
			emit log(chr_, "Getting " + QString::number(variant_query.size()) + " variants for " + chr_ + " took " + getTimeString(tmp_timer.nsecsElapsed()/1000000.0));

			// iterate over all variants
			while(variant_query.next())
			{
				QElapsedTimer v_timer;
				if (params_.verbose) v_timer.start();

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

				//check that coordinates are inside the chromosome
				if (variant.start()>reference_file.lengthOf(variant.chr()))
				{
					if (params_.verbose) emit log(chr_, "Variant " + variant.toString() + " skipped because chromosomal position is after chromosome end!");
					continue;
				}

				//convert to VCF format (prepend ref base)
				tmp_timer.restart();
				VcfLine vcf_line = variant.toVCF(reference_file);
				variant.setStart(vcf_line.start());
				variant.setRef(vcf_line.ref());
				variant.setObs(vcf_line.altString());
				ref_lookup_sum += tmp_timer.nsecsElapsed()/1000000.0;

				//output
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

				if(gnomad.toDouble() <= params_.max_af)
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
						if (!shared_data_.ps_infos.contains(ps_id)) continue;

						//ignore bad processed samples
						const ProcessedSampleInfo& info = shared_data_.ps_infos[ps_id];
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
								samples_done_het.unite(db.sameSamples(info.s_id, SameSampleMode::SAME_PATIENT));

								if (info.affected)
								{
									het_per_group[info.disease_group] += 1;
								}
							}
							if (mosaic && !samples_done_mosaic.contains(info.s_id))
							{
								++count_mosaic;
								samples_done_mosaic << info.s_id;
								samples_done_mosaic.unite(db.sameSamples(info.s_id, SameSampleMode::SAME_PATIENT));
							}
						}

						// count homozygous variants
						if (genotype == "hom" && !samples_done_hom.contains(info.s_id))
						{
							++count_hom;
							samples_done_hom << info.s_id;
							samples_done_hom.unite(db.sameSamples(info.s_id, SameSampleMode::SAME_PATIENT));

							if (info.affected)
							{
								hom_per_group[info.disease_group] += 1;
							}
						}
					}
					ngsd_count_calculation_sum += tmp_timer.nsecsElapsed()/1000000.0;

					// store counts in vcf
					info_column.append("COUNTS=" + QByteArray::number(count_hom) + "," + QByteArray::number(count_het) + "," + QByteArray::number(count_mosaic));


					QStringList disease_groups = db.getEnum("sample", "disease_group");
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
						count_cache.insert(variant_id, GenotypeCounts{count_hom, count_het, count_mosaic});
						if (count_cache.count()>=10000)
						{
							tmp_timer.restart();
							storeCountCache(db, count_cache);
							ngsd_count_update += tmp_timer.nsecsElapsed()/1000000.0;
						}
					}
				}
				else
				{
					// mark variants with high allele frequeny
					info_column.append("HAF");
				}

				// get classification
				if (shared_data_.class_infos.contains(variant_id))
				{
					QByteArray classification = shared_data_.class_infos[variant_id].classification;
					if (classification != "") info_column.append("CLAS=" + classification);
					QByteArray clas_comment = shared_data_.class_infos[variant_id].comment;
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
				++vcf_lines_written;
				if (vcf_lines_written%10000==0) vcf_stream.flush(); //flush VCF stream from time to time to make monitoring the progress possible
				vcf_file_writing_sum += tmp_timer.nsecsElapsed()/1000000.0;

				if (params_.verbose) emit log(chr_, variant.toString(false) + " gnomAD=" + gnomad + " time=" + getTimeString(v_timer.elapsed()));

				if (params_.max_vcf_lines>0 && vcf_lines_written>=params_.max_vcf_lines) break;
			}

			//store remaining entries in cache
			storeCountCache(db, count_cache);

			// close vcf file
			vcf_stream.flush();
			vcf_file->close();

			emit log(chr_, "Finished germline export");
			emit log(chr_, QString::number(vcf_lines_written) + " variants exported");
			emit log(chr_, "Time overall: " + getTimeString(chr_timer.elapsed()));
			emit log(chr_, "Time for ref sequence lookup: " + getTimeString(ref_lookup_sum));
			emit log(chr_, "Time for VCF writing: " + getTimeString(vcf_file_writing_sum));
			emit log(chr_, "Time for database queries (variant counts): " + getTimeString(ngsd_count_query_sum));
			emit log(chr_, "Time for calcuations (variant counts): " + getTimeString(ngsd_count_calculation_sum));
			emit log(chr_, "Time for for database update (variant counts): " + getTimeString(ngsd_count_update));
		}

		if (!params_.somatic.isEmpty())
		{
			QString tmp_vcf = params_.tempVcf(chr_, "somatic");
			emit log(chr_, "Starting somatic export to " + tmp_vcf);

			QElapsedTimer chr_timer;
			chr_timer.start();
			QElapsedTimer tmp_timer;
			double ref_lookup_sum = 0;
			double count_computation_sum = 0;
			double db_query_sum = 0;
			double vcf_file_writing_sum = 0;

			// define query to get the NGSD counts for each variant
			SqlQuery ngsd_count_query = db.getQuery();
			ngsd_count_query.prepare("SELECT s.id, dsv.processed_sample_id_tumor, p.name FROM detected_somatic_variant as dsv, processed_sample ps, sample as s, project as p WHERE ps.project_id=p.id AND ps.quality!='bad' AND dsv.processed_sample_id_tumor=ps.id AND ps.sample_id=s.id AND s.tumor='1' AND dsv.variant_id=:0");

			//open output stream
			QSharedPointer<QFile> vcf_file = Helper::openFileForWriting(tmp_vcf, true);
			QTextStream vcf_stream(vcf_file.data());

			long long vcf_lines_written = 0;

			//get somatic variants on this chromosome
			tmp_timer.start();
			SqlQuery variant_query = db.getQuery();
			variant_query.exec("SELECT id, chr, start, end, ref, obs FROM variant WHERE chr='" + chr_ + "' ORDER BY start ASC, end ASC");
			emit log(chr_, "Getting " + QString::number(variant_query.size()) + " variants for " + chr_ + " took " + getTimeString(tmp_timer.nsecsElapsed()/1000000.0));

			while(variant_query.next())
			{
				int variant_id = variant_query.value(0).toInt();
				if (!shared_data_.somatic_variant_ids.contains(variant_id)) continue;

				Variant variant;
				variant.setChr(Chromosome(variant_query.value(1).toByteArray()));
				variant.setStart(variant_query.value(2).toInt());
				variant.setEnd(variant_query.value(3).toInt());
				variant.setRef(variant_query.value(4).toByteArray());
				variant.setObs(variant_query.value(5).toByteArray());

				//get counts
				tmp_timer.start();
				ngsd_count_query.bindValue(0, variant_id);
				ngsd_count_query.exec();
				db_query_sum += tmp_timer.nsecsElapsed()/1000000.0;

				//process variants
				tmp_timer.start();
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
				QByteArrayList info_column;
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

					if(params_.vicc_config_details)
					{
						QMap<QString, QString> config_details = data.configAsMap();
						for(auto it = config_details.begin() ; it != config_details.end(); ++it)
						{
							info_column.append("SOM_VICC_" + it.key().toUpper().toUtf8() + "=" + VcfFile::encodeInfoValue(it.value()).toUtf8());
						}
					}
				}
				count_computation_sum += tmp_timer.elapsed();


				// modify sequence if deletion or insertion occurs (to fit VCF specification)
				if ((variant.ref() == "-") || (variant.obs() == "-"))
				{
					// benchmark
					tmp_timer.start();

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
					ref_lookup_sum += tmp_timer.elapsed();
				}

				//write output line
				tmp_timer.start();
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
				vcf_file_writing_sum += tmp_timer.elapsed();

				vcf_lines_written++;

				if (vcf_lines_written%10000==0) vcf_stream.flush(); //flush VCF stream from time to time to make monitoring the progress possible

				if (params_.max_vcf_lines>0 && vcf_lines_written>=params_.max_vcf_lines) break;
			}

			// close vcf file
			vcf_stream.flush();
			vcf_file->close();

			emit log(chr_, "Finished somatic export");
			emit log(chr_, QString::number(vcf_lines_written) + " variants exported");
			emit log(chr_, "Time overall: " + getTimeString(chr_timer.elapsed()));
			emit log(chr_, "Time for ref sequence lookup: " + getTimeString(ref_lookup_sum));
			emit log(chr_, "Time for VCF writing: " + getTimeString(vcf_file_writing_sum));
			emit log(chr_, "Time for database queries (variant counts): " + getTimeString(db_query_sum));
			emit log(chr_, "Time for calcuations (variant counts): " + getTimeString(count_computation_sum));
		}

		//signal that the chromosome is done
		emit done(chr_);
	}
	catch(Exception& e)
	{
		//QTextStream(stdout) << "ExportWorker:error " << chr_ << " message:" << e.message() << endl;
		emit error(chr_, e.message());
	}
}


//Function that stores cached variant counts
void ExportWorker::storeCountCache(NGSD& db, QHash<int, GenotypeCounts>& count_cache)
{
	QElapsedTimer timer;
	timer.start();

	//update counts
	int tries_max = 10;
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
			if (params_.verbose) emit log(chr_, "Count update transaction failed in try number " + QString::number(try_nr) + ": " + e.message());
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

	emit log(chr_, "Updating " + QString::number(count_cache.count()) + " variant counts took " + getTimeString(timer.nsecsElapsed()/1000000.0));

	//clear cache
	count_cache.clear();
}
