#include "GeneBurdenTest.h"
#include "VariantHgvsAnnotator.h"
#include "qjsonobject.h"

#include <Settings.h>


WorkerGeneBurdenTest::WorkerGeneBurdenTest(BurdenTestResult& gene_result, const BurdenTestParameters& parameters, const QMap<QByteArray,BedFile>& ccr80_region, const QSet<int>& ps_ids_cases, const QSet<int>& ps_ids_controls,
										   const QByteArrayList& ps_ids, const QSet<int>& callset_ids_cases, const QSet<int>& callset_ids_controls, const BedFile &cnv_polymorphism_region, bool test,  bool debug)
	: QRunnable()
	, gene_result_(gene_result)
	, parameters_(parameters)
	, ccr80_region_(ccr80_region)
	, ps_ids_cases_(ps_ids_cases)
	, ps_ids_controls_(ps_ids_controls)
	, ps_ids_(ps_ids)
	, callset_ids_cases_(callset_ids_cases)
	, callset_ids_controls_(callset_ids_controls)
	, cnv_polymorphism_region_(cnv_polymorphism_region)
	, test_(test)
	, debug_(debug)
{
	// db_ = new NGSD(test);
}

void WorkerGeneBurdenTest::run()
{
	try
	{
		NGSD db(test_);
		db.enableDebugging(debug_);
		QElapsedTimer timer;
		timer.start();
		if (debug_) QTextStream(stdout) << "Processing gene " << gene_result_.gene << Qt::endl;

		// get gene region
		BedFile gene_regions;
		if(parameters_.ccr_only)
		{
			//limit region only to CCR80
			if(ccr80_region_.contains(gene_result_.gene)) gene_regions = ccr80_region_.value(gene_result_.gene);
		}
		else
		{
			//use exon region
			gene_regions = db.geneToRegions(gene_result_.gene, Transcript::ENSEMBL, "exon", true);

			//extend exon region by splice region
			gene_regions.extend(parameters_.splice_region_size);
		}

		gene_regions.sort();
		gene_regions.merge();

		// qDebug() << gene_result_.gene << gene_regions.count();

		//skip genes without ensemble regions
		if (gene_regions.count() == 0)
		{
			gene_result_.p_value = 1.0; //Set p-value to 1 since no fisher test is performed
			gene_result_.warning = "Gene " + gene_result_.gene + " skipped cause it has no chromosomal regions!";
			return;
		}


		//get all variants for this gene
		QMap<int, Variant> variants = getVariantsForRegion(gene_regions);
		if (debug_) QTextStream(stdout) << "\t getting initial variants (" << variants.size() << ") took " << Helper::elapsedTime(timer) << Qt::endl;


		//filter by impact
		TranscriptList relevant_transcripts = db.relevantTranscripts(db.geneId(gene_result_.gene));
		relevant_transcripts.sortByPosition();
		ChromosomalIndex<TranscriptList> transcript_index(relevant_transcripts);
		FastaFileIndex genome_reference(Settings::string("reference_genome", false));
		VariantHgvsAnnotator hgvs_annotator(genome_reference);
		QSet<int> variant_ids;

		//debug stats
		int n_skipped_impact = 0;
		int n_skipped_non_pathogenic = 0;


		for (auto i = variants.cbegin(), end = variants.cend(); i != end; ++i)
		{
			const Variant& var = i.value();
			//get impact

			// add 5kb offset
			QVector<int> indices = transcript_index.matchingIndices(var.chr(), std::max(var.start()-5000, 0), var.end()+5000);

			QSet<VariantImpact> found_impacts;

			//no transcripts in proximity: intergenic variant
			if(indices.isEmpty()) found_impacts << VariantHgvsAnnotator::consequenceTypeToImpact(VariantConsequenceType::INTERGENIC_VARIANT);

			//get impact for each matching transcript
			foreach(int idx, indices)
			{
				const Transcript& t = relevant_transcripts.at(idx);
				//ignore variants which couldn't be annotated
				try
				{
					found_impacts << hgvs_annotator.annotate(t, var).impact;
				}
				catch (Exception e)
				{
					gene_result_.warning = "Variant " + var.toString(' ') + " could not be annotated! (" + e.message() + ")";
				}
			}

			// remove not requested impacts
			found_impacts = found_impacts.intersect(parameters_.impacts);

			// no matches found
			if (found_impacts.isEmpty())
			{
				n_skipped_impact++;
				continue;
			}
			// skip variants with impact < HIGH CADD < 20 and SpliceAI < 0.5
			if (parameters_.predict_pathogenic && !found_impacts.contains(VariantImpact::HIGH))
			{
				//check CADD/SpliceAI score
				double cadd = Helper::toDouble(var.annotations()[0], "CADD score", var.toString('-'));
				double spliceai = Helper::toDouble(var.annotations()[1], "SpliceAI score", var.toString('-'));

				if ((cadd < 20.0) && (spliceai < 0.5))
				{
					n_skipped_non_pathogenic++;
					continue;
				}
			}

			variant_ids << i.key();
		}

		if (debug_)
		{
			QTextStream(stdout) << "\t skipped wrong impact|non-pathogenic prediction: " << n_skipped_impact
								<< "|" << n_skipped_non_pathogenic << "\n\t\t -> remaining variants:" << variant_ids.size() << Qt::endl;
			QTextStream(stdout) << "\tFiltering variants took " << Helper::elapsedTime(timer) << Qt::endl;
		}


		// for all matching variants: get counts of case and control cohort
		QMap<int,QSet<int>> detected_variants;
		if(variant_ids.size() != 0)
		{
			QStringList var_ids_str;
			foreach (int id, variant_ids)
			{
				var_ids_str << QString::number(id);
			}
			SqlQuery detected_variant_query = db.getQuery();
			detected_variant_query.setForwardOnly(true);
			detected_variant_query.exec("SELECT processed_sample_id, variant_id FROM detected_variant WHERE variant_id IN (" + var_ids_str.join(", ") + ") "
										+ "AND processed_sample_id IN (" + ps_ids_.join(", ") + ")" + ((parameters_.include_mosaic)?"":" AND mosaic=0"));
			while (detected_variant_query.next())
			{
				int ps_id = detected_variant_query.value("processed_sample_id").toInt();
				int var_id = detected_variant_query.value("variant_id").toInt();

				detected_variants[ps_id] << var_id;
			}
		}

		gene_result_.hits_cases = getOccurences(variant_ids, ps_ids_cases_, detected_variants, parameters_.inheritance);
		gene_result_.hits_controls = getOccurences(variant_ids, ps_ids_controls_, detected_variants, parameters_.inheritance);

		if (parameters_.include_cnvs)
		{
			//get cnv counts
			if (callset_ids_cases_.size() > 0 ) gene_result_.hits_cases_cnv = getOccurencesCNV(callset_ids_cases_, gene_regions);
			if (callset_ids_controls_.size() > 0 ) gene_result_.hits_controls_cnv = getOccurencesCNV(callset_ids_controls_, gene_regions);
			//sort processed samples

			//get combined counts
			int n_cases_combined = (Helper::listToSet(gene_result_.hits_cases.keys()) + Helper::listToSet(gene_result_.hits_cases_cnv.keys())).size();
			int n_controls_combined = (Helper::listToSet(gene_result_.hits_controls.keys()) + Helper::listToSet(gene_result_.hits_controls_cnv.keys())).size();

			//calculate p-value (fisher) (SNPs only)
			gene_result_.p_value = BasicStatistics::fishersExactTest(n_cases_combined, n_controls_combined, ps_ids_cases_.size() - n_cases_combined, ps_ids_controls_.size() - n_controls_combined, "greater");
		}
		else
		{
			//calculate p-value (fisher) (SNPs only)
			gene_result_.p_value = BasicStatistics::fishersExactTest(gene_result_.hits_cases.size(), gene_result_.hits_controls.size(), ps_ids_cases_.size() - gene_result_.hits_cases.size(), ps_ids_controls_.size() - gene_result_.hits_controls.size(), "greater");
		}

		//debug output
		if (debug_) QTextStream(stdout) << "Processing gene " << gene_result_.gene <<  " took " << Helper::elapsedTime(timer) << Qt::endl;
	}
	catch(Exception& e)
	{
		gene_result_.error = e.message();
	}
	catch(std::exception& e)
	{
		gene_result_.error = e.what();
	}
	catch(...)
	{
		gene_result_.error = "Unknown exception!";
	}

}

QMap<int, Variant> WorkerGeneBurdenTest::getVariantsForRegion(const BedFile &regions)
{
	NGSD db(test_);
	db.enableDebugging(debug_);
	QMap<int, Variant> variants;

	if(regions.count() < 1)
	{
		THROW(ArgumentException, "BED file doesn't contain any regions!");
	}

	//execute query
	QString query_text = createGeneQuery(regions);
	SqlQuery query = db.getQuery();
	query.setForwardOnly(true);
	query.exec(query_text);
	if (debug_) QTextStream(stdout) << "\t initial variants: " << query.size() << Qt::endl;

	while(query.next())
	{
		int variant_id = query.value("id").toInt();
		Variant variant;
		variant.setChr(Chromosome(query.value("chr").toString()));
		variant.setStart(query.value("start").toInt());
		variant.setEnd(query.value("end").toInt());
		variant.setRef(query.value("ref").toString().toUtf8());
		variant.setObs(query.value("obs").toString().toUtf8());
		variant.setAnnotations(QByteArrayList() << query.value("cadd").toString().toUtf8() << query.value("spliceai").toString().toUtf8());

		variants.insert(variant_id, variant);

		// //filter by impact
		// bool at_least_one_part_matches = false;
		// foreach(const QString& part, query.value("coding").toString().split(","))
		// {
		// 	//skip empty enties
		// 	int index = part.indexOf(':');
		// 	if (index==-1)
		// 	{
		// 		n_skipped_empty++;
		// 		continue;
		// 	}

		// 	//skip all entries which doesn't describe the current gene
		// 	if(!part.trimmed().startsWith(gene_symbol))
		// 	{
		// 		n_skipped_gene++;
		// 		continue;
		// 	}

		// 	//filter by impact
		// 	bool match = false;
		// 	foreach(const QString& impact, parameters_.impacts)
		// 	{
		// 		if (part.contains(impact))
		// 		{
		// 			if (parameters_.predict_pathogenic && (impact != "HIGH") && (query.value("cadd").toDouble() < 20) && (query.value("spliceai").toDouble() < 0.5))
		// 			{
		// 				n_skipped_non_pathogenic++;
		// 				continue;
		// 			}
		// 			match = true;
		// 			break;
		// 		}
		// 	}
		// 	if (match)
		// 	{
		// 		at_least_one_part_matches = true;
		// 		break;
		// 	}

		// 	//TODO: filter by live-calculated impact?
		// }

		// if (!at_least_one_part_matches)
		// {
		// 	n_skipped_impact++;
		// 	continue;
		// }

		// // return variant id
		// variant_ids.insert(query.value("id").toInt());
	}

	// if (debug_) QTextStream(stdout) << "\t skipped empty|wrong gene|wrong impact|non-pathogenic prediction: " << n_skipped_empty << "|" << n_skipped_gene
	// 						<< "|" << n_skipped_impact << "|" << n_skipped_non_pathogenic << "\n\t\t -> remaining variants:" << variant_ids.size() << Qt::endl;

	return variants;
}

QString WorkerGeneBurdenTest::createGeneQuery(const BedFile &regions)
{
	//prepare db queries
	QString query_text = "SELECT id, chr, start, end, ref, obs, cadd, spliceai FROM variant WHERE (germline_het>0 OR germline_hom>0) AND germline_het+germline_hom<=" + QString::number(parameters_.max_ngsd_count)
						 + " AND (gnomad IS NULL OR gnomad<=" + QString::number(parameters_.max_gnomad_af) + ")";

	//impacts
	// if(parameters_.impacts.size() > 0)
	// {
	// 	query_text += " AND (";
	// 	QStringList impact_query_statement;
	// 	foreach (const QString& impact, parameters_.impacts)
	// 	{
	// 		if (!parameters_.predict_pathogenic || impact == "HIGH")
	// 		{
	// 			impact_query_statement << "(coding LIKE '%" + impact + "%')";
	// 		}
	// 		else
	// 		{
	// 			impact_query_statement << "(coding LIKE '%" + impact + "%' AND (cadd>=20 OR spliceai>=0.5))";
	// 		}
	// 	}
	// 	query_text += impact_query_statement.join(" OR ");
	// 	query_text += ")";
	// }

	//gene regions
	QStringList chr_ranges;
	Chromosome chr;
	for (int i = 0; i < regions.count(); ++i)
	{
		//set chromosome in first iteration
		if (!chr.isValid()) chr = regions[i].chr();
		// check if exons are all on same chromosome
		if (chr != regions[i].chr()) THROW(ArgumentException, "Exon regions of gene " + gene_result_.gene + " spann multiple chromosomes!");
		chr_ranges << "(end>=" + QString::number(regions[i].start()) + " AND start<=" + QString::number(regions[i].end()) + ")";
	}
	//collapse to final query
	query_text += " AND chr='" + chr.strNormalized(true) + "' AND (" + chr_ranges.join(" OR ") + ") ORDER BY start";

	return query_text;
}

QMap<QByteArray, QByteArray> WorkerGeneBurdenTest::getOccurences(const QSet<int> &variant_ids, const QSet<int> &ps_ids, const QMap<int, QSet<int> > &detected_variants, Inheritance inheritance)
{
	NGSD db(test_);
	db.enableDebugging(debug_);
	// load reference
	FastaFileIndex genome_reference(Settings::string("reference_genome", false));
	QMap<QByteArray, QByteArray> hits;
	foreach(int ps_id, ps_ids)
	{
		//check for variant in gene
		if(!detected_variants.contains(ps_id)) continue;
		QSet<int> intersection = variant_ids;
		intersection = intersection.intersect(detected_variants.value(ps_id));

		if (!parameters_.excluded_regions.isEmpty())
		{
			//filter matches based on blacklist BED file
			QSet<int> filtered_intersection;
			foreach (int variant_id, intersection)
			{
				Variant var = db.variant(QString::number(variant_id));
				if (!parameters_.excluded_regions.overlapsWith(var.chr(), var.start(), var.end())) filtered_intersection.insert(variant_id);
			}
			//update id list
			intersection = filtered_intersection;
		}

		//no match
		if (intersection.size() == 0) continue;

		//check for de-novo
		if(inheritance == Inheritance::de_novo)
		{
			int rc_id = db.reportConfigId(QString::number(ps_id));

			// no rc_id -> no de-novo variants
			if(rc_id < 0) continue;

			//get all de-novo variants of this sample
			QSet<int> de_novo_var_ids = Helper::listToSet(db.getValuesInt("SELECT variant_id FROM report_configuration_variant WHERE de_novo=TRUE AND report_configuration_id=:0", QString::number(rc_id)));
			intersection = intersection.intersect(de_novo_var_ids);

			//no de-novo variants
			if (intersection.size() == 0) continue;
		}
		else if((inheritance == Inheritance::recessive) && (intersection.size() == 1))
		{
			int variant_id = intersection.values().at(0);
			// check for hom var
			QString genotype = db.getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id=" + QString::number(ps_id) + " AND variant_id="
											+ QString::number(variant_id)).toString();

			// skip het vars (except het on chr X in male samples)
			if (genotype == "het")
			{
				QString gender = db.getSampleData(db.sampleId(db.processedSampleName(QString::number(ps_id)))).gender;
				if (gender != "male") continue;
				Variant var = db.variant(QString::number(variant_id));
				if (!var.chr().isX()) continue;
				BedFile par = NGSHelper::pseudoAutosomalRegion(GenomeBuild::HG38);
				if (!par.overlapsWith(var.chr(), var.start(), var.end())) continue;

				//else: variant is kept since it is a het variant on the chr X of a male sample
			}
		}
		//else: at least two hits or non-ressesive:

		//log sample name and variants
		QByteArray ps_name = db.processedSampleName(QString::number(ps_id)).toUtf8();

		QByteArrayList variants_per_sample;
		foreach (int var_id, intersection)
		{
			variants_per_sample << db.variant(QString::number(var_id)).toVCF(genome_reference).toString();
		}
		std::sort(variants_per_sample.begin(), variants_per_sample.end());
		hits[ps_name] = variants_per_sample.join(';');
	}

	return hits;
}

QMap<QByteArray, QByteArray> WorkerGeneBurdenTest::getOccurencesCNV(const QSet<int> &callset_ids, const BedFile &regions)
{

	NGSD db(test_);
	db.enableDebugging(debug_);
	QMap<QByteArray, QByteArray> ps_names;
	ChromosomalIndex<BedFile> cnv_polymorphism_region_index(cnv_polymorphism_region_);

	//get all cnvs intersecting the given region
	//create query
	QString query_text = QString("SELECT id FROM cnv WHERE ");

	//filter by callset
	QStringList callset_str;
	foreach (int c_id, callset_ids)
	{
		callset_str << QString::number(c_id);
	}
	query_text += "cnv_callset_id IN (" +  callset_str.join(", ") + ") AND ";

	//filter by region
	QStringList chr_ranges;
	Chromosome chr;
	for (int i = 0; i < regions.count(); ++i)
	{
		//set chromosome in first iteration
		if (!chr.isValid()) chr = regions[i].chr();
		// check if exons are all on same chromosome
		if (chr != regions[i].chr()) THROW(ArgumentException, "Exon regions of gene " + gene_result_.gene + " spann multiple chromosomes!");
		chr_ranges << "(end>=" + QString::number(regions[i].start()) + " AND start<=" + QString::number(regions[i].end()) + ")";
	}
	query_text += "(" + chr_ranges.join(" OR ") + ")";

	//execute
	QList<int> cnv_ids = db.getValuesInt(query_text);

	//filter down variants
	foreach (int cnv_id, cnv_ids)
	{
		//filter by CN
		int cn = db.getValue("SELECT cn FROM cnv WHERE id=:0", false, QString::number(cnv_id)).toInt();
		if ((parameters_.inheritance == Inheritance::recessive) && (cn != 0)) continue;
		if  (cn > 1) continue; // in dominant/de-novo case

		//filter by logll
		QJsonDocument json = QJsonDocument::fromJson(db.getValue("SELECT cn FROM cnv WHERE id=:0", false, QString::number(cnv_id)).toByteArray());
		int ll = json.object().value("loglikelihood").toInt();
		int n_regions = (json.object().contains("regions"))?json.object().value("regions").toInt():json.object().value("no_of_regions").toInt();
		double scaled_ll = (double) ll / n_regions;
		if (scaled_ll < 15.0) continue;

		//filter by polymorphim region
		CopyNumberVariant cnv = db.cnv(cnv_id);
		QVector<int> indices = cnv_polymorphism_region_index.matchingIndices(cnv.chr(), cnv.start(), cnv.end());
		BedFile overlap_regions;
		foreach (int idx, indices)
		{
			const BedLine& match = cnv_polymorphism_region_[idx];
			overlap_regions.append(BedLine(cnv.chr(), std::max(cnv.start(),match.start()), std::min(cnv.end(),match.end())));
		}
		overlap_regions.sort();
		overlap_regions.merge();
		double overlap = (double) overlap_regions.baseCount() / cnv.size();
		if(overlap > 0.95) continue;

		int ps_id = db.getValue("SELECT cc.processed_sample_id FROM cnv c INNER JOIN cnv_callset cc ON cc.id=c.cnv_callset_id WHERE c.id=:0", false, QString::number(cnv_id)).toInt();

		//log sample name and CNV
		QByteArray ps_name = db.processedSampleName(QString::number(ps_id)).toUtf8();
		if (ps_names.contains(ps_name))
		{
			ps_names[ps_name] += ";" + cnv.toString().toUtf8();
		}
		else
		{
			ps_names[ps_name] = cnv.toString().toUtf8();
		}

		//sort results
		QByteArrayList tmp = ps_names[ps_name].split(';');
		std::sort(tmp.begin(), tmp.end());
		ps_names[ps_name] = tmp.join(';');
	}

	//report results
	return ps_names;
}



GeneBurdenTest::GeneBurdenTest(const QSet<int> &ps_ids_cases, const QSet<int> &ps_ids_controls, const GeneSet &genes, BurdenTestParameters parameters, int threads, bool test, bool debug, bool skip_errors)
	: ps_ids_cases_(ps_ids_cases)
	, ps_ids_controls_(ps_ids_controls)
	, genes_(genes)
	, parameters_(parameters)
	, db_(test)
	, test_(test)
	, threads_(threads)
	, debug_(debug)
	, skip_errors_(skip_errors)
{
	//init CCR region
	if (parameters_.ccr_only) initCCR();

	//convert genes to approved symbols:
	genes_ = db_.genesToApproved(genes_);
}

QList<BurdenTestResult> GeneBurdenTest::run_burden_test()
{


	QElapsedTimer timer;
	timer.start();

	//get all processed sample ids
	foreach (int id, ps_ids_cases_ + ps_ids_controls_)
	{
		ps_ids_ << QByteArray::number(id);
	}

	//Debug:
	if (debug_)
	{
		QTextStream(stdout) << "case samples: " << ps_ids_cases_.size() << Qt::endl;
		QTextStream(stdout) << "control samples: " << ps_ids_controls_.size() << Qt::endl;
		QTextStream(stdout) << "combined list: " << ps_ids_.size() << Qt::endl;
	}

	//Sanity check
	if ((ps_ids_cases_.size() + ps_ids_controls_.size()) != ps_ids_.size()) THROW(ArgumentException, "Combined ID size don't match cohort sizes!");



	if (parameters_.include_cnvs)
	{
		//get callset ids for each processed sample
		SqlQuery cnv_callset_query = db_.getQuery();
		cnv_callset_query.prepare("SELECT id, quality_metrics FROM cnv_callset WHERE processed_sample_id=:0");
		for (int ps_id : std::as_const(ps_ids_cases_))
		{
			//get sample type
			QString processing_system_type = db_.getProcessedSampleData(QString::number(ps_id)).processing_system_type;
			double min_correlation = (processing_system_type == "WGS") ? 0.35: 0.9;
			cnv_callset_query.bindValue(0, ps_id);
			cnv_callset_query.setForwardOnly(true);
			cnv_callset_query.exec();
			while(cnv_callset_query.next())
			{
				int callset_id = cnv_callset_query.value(0).toInt();
				QMap<QString, QVariant> quality_metrics = QJsonDocument::fromJson(cnv_callset_query.value(1).toByteArray()).object().toVariantMap();
				double ref_correlation = quality_metrics.value("mean correlation to reference samples").toDouble();
				if (ref_correlation >= min_correlation) callset_ids_cases_ << callset_id;
			}
		}
		for (int ps_id : std::as_const(ps_ids_controls_))
		{
			//get sample type
			QString processing_system_type = db_.getProcessedSampleData(QString::number(ps_id)).processing_system_type;
			double min_correlation = (processing_system_type == "WGS") ? 0.35: 0.9;
			cnv_callset_query.bindValue(0, ps_id);
			cnv_callset_query.setForwardOnly(true);
			cnv_callset_query.exec();
			while(cnv_callset_query.next())
			{
				int callset_id = cnv_callset_query.value(0).toInt();
				QJsonDocument quality_metrics = QJsonDocument::fromJson(cnv_callset_query.value(1).toByteArray());
				double ref_correlation = quality_metrics.object().value("mean correlation to reference samples").toDouble();

				if (ref_correlation >= min_correlation) callset_ids_controls_ << callset_id;
			}
		}

		if (debug_)
		{
			QTextStream(stdout) << "callset ids cases:" << callset_ids_cases_.size() << Qt::endl;
			QTextStream(stdout) << "callset ids controls:" << callset_ids_controls_.size() << Qt::endl;
		}

		//read CNV polymorphism region
		if (test_)
		{
			// use pre-defined test file
			QTextStream(stderr) << "Running in test mode, using predefined polymorphism region" << Qt::endl;
			cnv_polymorphism_region_.load(":/resources/GeneBurdenTest_cnv_af.bed", false);
		}
		else
		{
			QString filepath = Settings::string("burden_test_cnp_regions", true);
			if (filepath.isEmpty())
			{
				QTextStream(stderr) << "WARNING: CNV polymorphism file not set! (burden_test_cnp_regions in settings ini)" << Qt::endl;
			}
			else
			{
				cnv_polymorphism_region_.load(filepath, false);
				cnv_polymorphism_region_.sort();
			}
		}


	}

	//create container for analysis
	QList<BurdenTestResult> result;
	for (const QByteArray& gene : std::as_const(genes_))
	{
		BurdenTestResult gene_result;
		gene_result.gene = gene;
		result << gene_result;
	}

	//create thread pool
	QThreadPool thread_pool;
	thread_pool.setMaxThreadCount(threads_);

	//start analysis chunks
	for (int i=0; i<result.count(); ++i)
	{
		WorkerGeneBurdenTest* worker = new WorkerGeneBurdenTest(result[i], parameters_, ccr80_region_, ps_ids_cases_, ps_ids_controls_, ps_ids_, callset_ids_cases_, callset_ids_controls_, cnv_polymorphism_region_, test_, debug_);
		thread_pool.start(worker);
	}

	//wait until finished
	if (debug_) QTextStream(stdout) << "Waiting for workers to finish..." << Qt::endl;
	thread_pool.waitForDone();


	//debug output
	if (debug_) QTextStream(stdout) << "Processing genes took " << Helper::elapsedTime(timer) << Qt::endl;

	//check for errors
	QList<int> gene_indices_to_remove;
	for (int i=0; i<result.count(); ++i)
	{
		if (!result[i].error.isEmpty())
		{
			if (skip_errors_)
			{
				QTextStream(stdout) << "ERROR in processing gene " + result[i].gene + ":\t" + result[i].error << "\t Removed from output!" << Qt::endl;
				gene_indices_to_remove << i;
			}
			else
			{
				THROW(Exception, "ERROR in processing gene " + result[i].gene + ":\t" + result[i].error);
			}

		}
		if (!result[i].warning.isEmpty()) QTextStream(stdout) << "WARNING in processing gene " << result[i].gene << ":\t" << result[i].warning << Qt::endl;
	}

	//remove failed genes
	std::reverse(gene_indices_to_remove.begin(), gene_indices_to_remove.end());
	for (int i : std::as_const(gene_indices_to_remove))
	{
		result.removeAt(i);
	}

	// sort result by pValue
	std::sort(result.begin(), result.end());

	//debug output
	if (debug_) QTextStream(stdout) << "Burden test took " << Helper::elapsedTime(timer) << Qt::endl;

	return result;
}

void GeneBurdenTest::initCCR()
{
	QElapsedTimer timer;
	timer.start();

	//read CCR gene list and update the gene names
	ccr_genes_ = GeneSet::createFromFile(":/resources/CCR_supported_genes.txt");
	//debug output
	if (debug_) QTextStream(stdout) << "CCR gene file loading took " << Helper::elapsedTime(timer) << Qt::endl;

	//convert to approved in non-test mode (too slow for testing)
	if (!test_) ccr_genes_ = db_.genesToApproved(ccr_genes_);

	//debug output
	if (debug_) QTextStream(stdout) << "CCR gene validation took " << Helper::elapsedTime(timer) << Qt::endl;

	//create CCR region for each gene
	BedFile combined_ccr;
	combined_ccr.load(":/resources/CCR80_GRCh38.bed");
	ccr80_region_.clear();
	for (int i=0; i<combined_ccr.count(); i++)
	{
		const BedLine& line = combined_ccr[i];
		QByteArray gene = line.annotations().at(1).trimmed();
		//skip gene validation in test mode
		if (!test_) gene = db_.geneToApproved(line.annotations().at(1));
		//skip invalid genes
		if(gene.isEmpty()) continue;
		ccr80_region_[gene].append(BedLine(line.chr(), line.start(), line.end()));
	}

	//debug output
	if (debug_) QTextStream(stdout) << "CCR initialisation took " << Helper::elapsedTime(timer) << Qt::endl;
}





