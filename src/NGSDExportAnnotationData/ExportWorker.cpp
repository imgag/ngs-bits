#include "ExportWorker.h"
#include "Exceptions.h"
#include <QElapsedTimer>

ExportWorker::ExportWorker(QString chr, const GermlineParameters& params, const SharedData& shared_data)
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
	if (params_.verbose) emit log(chr_, "run");
	try
	{
		emit log(chr_, "Starting export");
		emit log(chr_, "Parameters: " + params_.toString());

		QHash<int, GenotypeCounts> count_cache;

		//init
		FastaFileIndex reference_file(params_.ref_file);
		NGSD db(params_.use_test_db);
		emit log(chr_, params_.use_test_db ? "TEST" : "NOT TEST");
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
		QSharedPointer<QFile> vcf_file = Helper::openFileForWriting(shared_data_.chr2vcf[chr_], true);
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

			//prepend reference base required in VCF to insertions/deletions
			if ((variant.ref() == "-") || (variant.obs() == "-"))
			{
				//check that coordinates are inside the chromosome
				if (variant.start()>reference_file.lengthOf(variant.chr()))
				{
					if (params_.verbose) emit log(chr_, "Variant " + variant.toString() + " skipped because chromosomal position is after chromosome end!");
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
			if (vcf_lines_written%1000==0) vcf_stream.flush(); //flush VCF stream from time to time to make monitoring the progress possible
			vcf_file_writing_sum += tmp_timer.nsecsElapsed()/1000000.0;

			if (params_.verbose) emit log(chr_, variant.toString(false) + " gnomAD=" + gnomad + " time=" + getTimeString(v_timer.elapsed()));

			if (params_.max_vcf_lines>0 && vcf_lines_written>=params_.max_vcf_lines) break;
		}

		emit log(chr_, "Finished export");
		emit log(chr_, QString::number(vcf_lines_written) + " variants exported");
		emit log(chr_, "Time overall: " + getTimeString(chr_timer.elapsed()));

		emit log(chr_, "Time for ref sequence lookup: " + getTimeString(ref_lookup_sum));

		emit log(chr_, "Time for VCF writing: " + getTimeString(vcf_file_writing_sum));
		emit log(chr_, "Time for database queries (variant counts): " + getTimeString(ngsd_count_query_sum));
		emit log(chr_, "Time for genotype calcuations (variant counts): " + getTimeString(ngsd_count_calculation_sum));
		emit log(chr_, "Time for for database update (variant counts): " + getTimeString(ngsd_count_update));

		//store remaining entries in cache
		storeCountCache(db, count_cache);

		// close vcf file
		vcf_stream.flush();
		vcf_file->close();

		emit log(chr_, "Starting finished");
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
