#include "ToolBase.h"
#include "NGSD.h"
#include "Auxilary.h"
#include "ThreadCoordinator.h"
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
		setDescription("Export information aboug germline variants, somatic variants and genes form NGSD for use as annotation source, e.g. in megSAP.");
		addOutfile("variants", "Output variant list as VCF.", false, true);

		//optional
		addInfile("reference", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addEnum("mode", "Determines the database which is exported.", true, QStringList() << "germline" << "somatic", "germline");
		addFloat("max_af", "Maximum allel frequency of exported variants (germline mode).",  true, 0.05);
		addOutfile("genes", "Also exports BED file containing genes and gene info (germline mode).", true, false);
		addInt("gene_offset", "Defines the number of bases by which the regions of genes are extended (germline mode).", true, 5000);
		addFlag("vicc_config_details", "Includes details about VICC interpretation (somatic mode).");
		addInt("threads", "Number of threads to use (germline mode).", true, 10);
		addFlag("verbose", "Enables verbose debug output (germline mode).");
		addInt("max_vcf_lines", "Maximum number of VCF lines to write per chromosome - for debugging (germline mode)", true, -1);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2023,  6, 16, "Added support for 'germline_mosaic' column in 'variant' table and added parallelization.");
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
		QString ref_file = getInfile("reference");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		QString variants = getOutfile("variants");

		//annotate
		QString mode = getEnum("mode");
		if(mode=="germline")
		{
			GermlineParameters params;
			params.ref_file = ref_file;
			params.output_file = variants;
			params.max_af = getFloat("max_af");
			if (params.max_af < 0) THROW(CommandLineParsingException, "Maximum AF has to be a positive value!");
			params.max_vcf_lines = getInt("max_vcf_lines");
			params.threads = getInt("threads");
			if (params.threads < 0) THROW(CommandLineParsingException, "Number of threads has to be a positive value!");
			params.genes = getOutfile("genes");
			params.gene_offset = getInt("gene_offset");
			if (params.gene_offset < 0) THROW(CommandLineParsingException, "Gene offset has to be a positive value!");
			params.use_test_db = getFlag("test");
			params.verbose = getFlag("verbose");
			params.version = version();
			ThreadCoordinator* coordinator = new ThreadCoordinator(this, params);
			connect(coordinator, SIGNAL(finished()), QCoreApplication::instance(), SLOT(quit()));
			setExitEventLoopAfterMain(false);
		}
		else if(mode=="somatic")
		{
			NGSD db(getFlag("test"));
			exportingVariantsToVcfSomatic(ref_file, variants, db, getFlag("vicc_config_details"));
		}
	}

private:

	//writes the germline variant annotation data from the NGSD to a vcf file
	void exportingVariantsToVcfSomatic(QString reference_file_path, QString vcf_file_path, NGSD& db, bool vicc_config_details)
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

		if(vicc_config_details)
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

						if(vicc_config_details)
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
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
