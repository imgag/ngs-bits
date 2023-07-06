#include "VariantScores.h"
#include "Exceptions.h"
#include "Helper.h"
#include "GeneSet.h"
#include "FilterCascade.h"
#include "Settings.h"

VariantScores::VariantScores()
{
}

QStringList VariantScores::algorithms()
{
	static QStringList algorithms;
	if (algorithms.isEmpty())
	{
		algorithms << "GSvar_v1";
		algorithms << "GSvar_v2_dominant";
		algorithms << "GSvar_v2_recessive";
	}
	return algorithms;
}

QString VariantScores::description(QString algorithm)
{
	if (!algorithms().contains(algorithm))
	{
		THROW(ArgumentException, "VariantScores::description: Unregistered algorithm name '" + algorithm + "'!");
	}

	if (algorithm=="GSvar_v1")
	{
		return "Variant ranking based on clinical information only.";
	}
	if (algorithm=="GSvar_v2_dominant")
	{
		return "Variant ranking based on clinical information only (dominant model)";
	}
	if (algorithm=="GSvar_v2_recessive")
	{
		return "Variant ranking based on clinical information only (recessive model)";
	}

	THROW(ArgumentException, "VariantScores::description: Not implemented algorithm '" + algorithm + "'!");
}

VariantScores::Result VariantScores::score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters)
{
	if (!algorithms().contains(algorithm))
	{
		THROW(ArgumentException, "VariantScores: Unregistered algorithm name '" + algorithm + "'!");
	}

	//score
	VariantScores::Result result;
	if (algorithm=="GSvar_v1")
	{
		 result = score_GSvar_v1(variants, phenotype_rois, parameters);
	}
	else if (algorithm=="GSvar_v2_dominant")
	{
		result = score_GSvar_v2_dominant(variants, phenotype_rois, parameters);
	}
	else if (algorithm=="GSvar_v2_recessive")
	{
		result = score_GSvar_v2_recessive(variants, phenotype_rois, parameters);
	}
	else
	{
		THROW(ArgumentException, "VariantScores: Not implemented algorithm '" + algorithm + "'!");
	}

	//calculate ranks
	struct IndexScorePair
	{
		int index;
		double score;
	};
	QList<IndexScorePair> tmp;
	for(int i=0; i<result.scores.count(); ++i)
	{
		tmp << IndexScorePair{i, result.scores[i]};
		result.ranks << -1;
	}
	std::stable_sort(tmp.begin(), tmp.end(), [](const IndexScorePair& a, const IndexScorePair& b){ return a.score > b.score; });
	for(int i=0; i<tmp.count(); ++i)
	{
		if (tmp[i].score>=0)
		{
			result.ranks[tmp[i].index] = i+1;
		}
	}

	result.algorithm = algorithm;
	return result;
}

int VariantScores::annotate(VariantList& variants, const VariantScores::Result& result, bool add_explanations)
{
	//check input
	if (variants.count()!=result.scores.count()) THROW(ProgrammingException, "Variant list and scoring result differ in count!");

	//add columns if missing
	if (add_explanations && variants.annotationIndexByName("GSvar_score_explanations", true, false)==-1)
	{
		variants.prependAnnotation("GSvar_score_explanations", "GSvar score explanations.");
	}
	if (variants.annotationIndexByName("GSvar_score", true, false)==-1)
	{
		variants.prependAnnotation("GSvar_score", "GSvar score (algorithm: " + result.algorithm + ", description:" + VariantScores::description(result.algorithm)+  ")");
	}
	if (variants.annotationIndexByName("GSvar_rank", true, false)==-1)
	{
		variants.prependAnnotation("GSvar_rank", "GSvar score based rank.");
	}
	int i_rank = variants.annotationIndexByName("GSvar_rank");
	int i_score = variants.annotationIndexByName("GSvar_score");
	int i_score_exp = add_explanations ? variants.annotationIndexByName("GSvar_score_explanations") : -1;

	//annotate
	int c_scored = 0;
	for (int i=0; i<variants.count(); ++i)
	{
		QByteArray score_str;
		QByteArray rank_str;
		if (result.scores[i] >= 0)
		{
			score_str = QByteArray::number(result.scores[i], 'f', 2);
			rank_str = QByteArray::number(result.ranks[i]);
			++c_scored;
		}
		variants[i].annotations()[i_score] = score_str;
		variants[i].annotations()[i_rank] = rank_str;
		if (add_explanations) variants[i].annotations()[i_score_exp] = result.score_explanations[i].join(" ").toUtf8();
	}

	return c_scored;
}

QList<Variant> VariantScores::loadBlacklist()
{
	QList<Variant> output;

	QStringList entries = Settings::stringList("ranking_variant_blacklist", true);
	foreach(const QString& entry, entries)
	{
		output << Variant::fromString(entry);
	}

	return output;
}

void CategorizedScores::add(const QByteArray& category, double value)
{
	add(category, value, "*");
}

void CategorizedScores::add(const QByteArray& category, double value, const QByteArray& gene)
{
	//init
	if (!operator[](gene).contains(category))
	{
		operator[](gene)[category] = 0.0;
	}

	operator[](gene)[category] = std::max(operator[](gene)[category], value);
}

double CategorizedScores::score(QByteArrayList& best_genes) const
{
	double output = 0.0;

	//gene-independent scores
	const QHash<QByteArray, double>& tmp = operator[]("*");
	for (auto it=tmp.begin(); it!=tmp.end(); ++it)
	{
		output += it.value();
	}

	//gene-specific scores
	QHash<QByteArray, double> gene2score;
	foreach(const QByteArray& gene, keys())
	{
		if (gene=="*") continue;

		const QHash<QByteArray, double>& tmp = operator[](gene);
		for (auto it=tmp.begin(); it!=tmp.end(); ++it)
		{
			gene2score[gene] += it.value();
		}
	}

	//determine maximum score
	double max_gene = 0.0;
	for (auto it=gene2score.begin(); it!=gene2score.end(); ++it)
	{
		if (it.value()>max_gene) max_gene = it.value();
	}

	//determine best genes
	best_genes.clear();
	for (auto it=gene2score.begin(); it!=gene2score.end(); ++it)
	{
		if (it.value()==max_gene) best_genes << it.key();
	}

	return output + max_gene;
}

QStringList CategorizedScores::explainations(const QByteArray& gene) const
{
	QStringList output;

	//gene-independent scores
	const QHash<QByteArray, double>& tmp = operator[]("*");
	for (auto it=tmp.begin(); it!=tmp.end(); ++it)
	{
		output << it.key() + ":" + QString::number(it.value(), 'f', 1);
	}

	//gene-specific scores
	if (!gene.isEmpty())
	{
		const QHash<QByteArray, double>& tmp2 = operator[](gene);
		for (auto it=tmp2.begin(); it!=tmp2.end(); ++it)
		{
			output << it.key() + ":" + QString::number(it.value(), 'f', 1);
		}
	}

	output.sort(Qt::CaseInsensitive);

	return output;
}

QStringList CategorizedScores::explainations(const QByteArrayList& best_genes) const
{
	if  (best_genes.isEmpty())
	{
		return explainations("");
	}

	QStringList output;
	foreach(const QByteArray& gene, best_genes)
	{
		if (best_genes.count()>1) output << "[" + gene + "]";
		output << explainations(gene);
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_v1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters)
{
	Result output;

	//get indices of annotations we need
	int i_coding = variants.annotationIndexByName("coding_and_splicing");
	int i_gnomad = variants.annotationIndexByName("gnomAD");
	int i_omim = variants.annotationIndexByName("OMIM", true, false);
	int i_hgmd = variants.annotationIndexByName("HGMD", true, false);
	int i_clinvar = variants.annotationIndexByName("ClinVar");
	int i_gene_info = variants.annotationIndexByName("gene_info");
	int i_classification = variants.annotationIndexByName("classification");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "VariantScores: Algorihtm 'GSvar_v1' can only be applied to variant lists with exactly one affected patient!");
	int i_genotye = affected_cols[0];

	//get blacklist variants
	QList<Variant> blacklist = loadBlacklist();

	//prepare ROI for fast lookup
	if (phenotype_rois.count()==0) output.warnings << "No phenotype region(s) set!";
	BedFile roi;
	foreach(const BedFile& pheno_roi, phenotype_rois)
	{
		roi.add(pheno_roi);
	}
	roi.merge();
	ChromosomalIndex<BedFile> roi_index(roi);

	//apply pre-filters to reduce runtime
	QStringList filters;
	filters << "Allele frequency	max_af=0.1"
			<< "Allele frequency (sub-populations)	max_af=0.1"
			<< "Variant quality	qual=30	depth=5	mapq=20	strand_bias=-1	allele_balance=-1"
			<< "Count NGSD	max_count=10	ignore_genotype=false	mosaic_as_het=false"
			<< "Impact	impact=HIGH,MODERATE,LOW"
			<< "Annotated pathogenic	action=KEEP	sources=HGMD,ClinVar	also_likely_pathogenic=false"
			<< "Allele frequency	max_af=1.0"
			<< "Filter columns	entries=mosaic	action=REMOVE"
			<< "Classification NGSD	action=REMOVE	classes=1,2";
	if (parameters.use_ngsd_classifications) filters << "Classification NGSD	action=KEEP	classes=4,5";
	FilterCascade cascade = FilterCascade::fromText(filters);
	FilterResult cascade_result = cascade.apply(variants);

	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << -1.0;
			output.score_explanations << QStringList();
			continue;
		}

		//skip blacklist variants
		const Variant& v = variants[i];
		if (parameters.use_blacklist && blacklist.contains(v))
		{
			output.scores << -2.0;
			output.score_explanations << QStringList();
			continue;
		}

		//get gene/transcript list
		QList<VariantTranscript> transcript_info = v.transcriptAnnotations(i_coding);
		GeneSet genes;
		foreach(const VariantTranscript& transcript, transcript_info)
		{
			genes << transcript.gene;
		}

		double score = 0.0;
		QStringList explanations;

		//in phenotype ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			score += 2.0;
			explanations << "HPO:2.0";
		}

		//impact
		double impact_score = 0.0;
		foreach(const VariantTranscript& transcript, transcript_info)
		{
			if(transcript.impact=="HIGH")
			{
				impact_score = std::max(impact_score, 3.0);
			}
			else if(transcript.impact=="MODERATE")
			{
				impact_score = std::max(impact_score, 2.0);
			}
			else if(transcript.impact=="LOW")
			{
				impact_score = std::max(impact_score, 1.0);
			}
		}
		if (impact_score>0)
		{
			score += impact_score;
			explanations << "impact:" + QString::number(impact_score, 'f', 1);
		}

		//gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			score += 1.0;
			explanations << "gnomAD:1.0";
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001)
			{
				score += 0.5;
				explanations << "gnomAD:0.5";
			}
		}

		//OMIM gene
		if (i_omim!=-1) //optional because of license
		{
			QByteArray omim = v.annotations()[i_omim].trimmed();
			if (!omim.isEmpty())
			{
				score += 1.0;
				explanations << "OMIM:1.0";
			}
		}

		//HGMD
		if (i_hgmd!=-1) //optional because of license
		{
			double hgmd_score = 0.0;
			QByteArrayList hgmd = v.annotations()[i_hgmd].trimmed().split(';');
			foreach(const QByteArray& entry, hgmd)
			{
				if (entry.contains("DM?"))
				{
					hgmd_score = std::max(hgmd_score, 0.3);
				}
				else if (entry.contains("DM"))
				{
					hgmd_score = std::max(hgmd_score, 0.5);
				}

			}
			if (hgmd_score>0)
			{
				score += hgmd_score;
				explanations << "HGMD:" + QString::number(hgmd_score, 'f', 1);
			}
		}

		//ClinVar
		double clinvar_score = 0.0;
		QByteArrayList clinvar = v.annotations()[i_clinvar].trimmed().split(';');
		foreach(const QByteArray& entry, clinvar)
		{
			if (entry.contains("likely pathogenic"))
			{
				clinvar_score = std::max(clinvar_score, 0.5);
			}
			else if (entry.contains("pathogenic"))
			{
				clinvar_score = std::max(clinvar_score, 1.0);
			}

		}
		if (clinvar_score>0)
		{
			score += clinvar_score;
			explanations << "ClinVar:" + QString::number(clinvar_score, 'f', 1);
		}

		//NGSD classification
		if (parameters.use_ngsd_classifications)
		{
			QByteArray classification = v.annotations()[i_classification].trimmed();
			if (classification=="4")
			{
				score += 0.5;
				explanations << "NGSD class:0.5";
			}
			if (classification=="5")
			{
				score += 1;
				explanations << "NGSD class:1.0";
			}
		}

		//genotype
		QByteArray genotype = v.annotations()[i_genotye].trimmed();
		if (genotype=="hom")
		{
			score += 1.0;
			explanations << "homozygous:1.0";
		}

		//gene-specific infos (gnomAD o/e lof, inheritance)
		bool inh_match = false;
		double min_oe = 1;
		QByteArrayList gene_infos = v.annotations()[i_gene_info].trimmed().split(',');

		foreach(const QByteArray& gene, genes)
		{
			foreach(const QByteArray& gene_info, gene_infos)
			{
				//use gene info for current gene only
				if (!gene_info.startsWith(gene+" ")) continue;

				int start = gene_info.indexOf('(');
				QByteArrayList entries = gene_info.mid(start+1, gene_info.length()-start-2).split(' ');
				foreach(const QByteArray& entry, entries)
				{
					if (entry.startsWith("inh="))
					{
						QByteArray mode = entry.split('=')[1].trimmed();

						if (
							(genotype=="het" && (mode.contains("AD") || mode.contains("XLD")))
							||
							(genotype=="hom" && (mode.contains("AR") || mode.contains("XLR")))
							)
						{
							inh_match = true;
						}
					}
					if (entry.startsWith("oe_lof="))
					{
						QByteArray oe = entry.split('=')[1].trimmed();
						if (oe!="n/a" && !oe.isEmpty())
						{
							min_oe = std::min(min_oe, Helper::toDouble(oe, "gnomAD o/e LOF"));
						}
					}
				}
			}
		}
		if (inh_match)
		{
			score += 0.5;
			explanations << "gene_inheritance:0.5";
		}
		if (min_oe<0.1)
		{
			score += 0.5;
			explanations << "gene_oe_lof:0.5";
		}

		output.scores << score;
		explanations.sort(Qt::CaseInsensitive);
		output.score_explanations << explanations;
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_v2_dominant(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters)
{
	Result output;

	//get indices of annotations we need
	int i_coding = variants.annotationIndexByName("coding_and_splicing");
	int i_gnomad = variants.annotationIndexByName("gnomAD");
	int i_ngsd_het = variants.annotationIndexByName("NGSD_het");
	int i_omim = variants.annotationIndexByName("OMIM", true, false);
	int i_hgmd = variants.annotationIndexByName("HGMD", true, false);
	int i_clinvar = variants.annotationIndexByName("ClinVar");
	int i_gene_info = variants.annotationIndexByName("gene_info");
	int i_classification = variants.annotationIndexByName("classification");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "VariantScores: Algorihtm 'GSvar_v1' can only be applied to variant lists with exactly one affected patient!");

	//get blacklist variants
	QList<Variant> blacklist = loadBlacklist();

	//prepare ROI for fast lookup
	if (phenotype_rois.count()==0) output.warnings << "No phenotype region(s) set!";
	BedFile roi;
	foreach(const BedFile& pheno_roi, phenotype_rois)
	{
		roi.add(pheno_roi);
	}
	roi.merge();
	ChromosomalIndex<BedFile> roi_index(roi);

	//apply pre-filters to reduce runtime
	QStringList filters;
	filters << "Allele frequency	max_af=0.1"
			<< "Allele frequency (sub-populations)	max_af=0.1"
			<< "Variant quality	qual=30	depth=1	mapq=20	strand_bias=-1	allele_balance=-1	min_occurences=0	min_af=0	max_af=1"
			<< "Count NGSD	max_count=10	ignore_genotype=false	mosaic_as_het=false"
			<< "Impact	impact=HIGH,MODERATE,LOW"
			<< "Annotated pathogenic	action=KEEP	sources=HGMD,ClinVar	also_likely_pathogenic=false"
			<< "Splice effect	MaxEntScan=LOW	SpliceAi=0.5	action=KEEP"
			<< "Allele frequency	max_af=1.0"
			<< "Filter columns	entries=mosaic	action=REMOVE"
			<< "Classification NGSD	action=REMOVE	classes=1,2";
	if (parameters.use_ngsd_classifications) filters << "Classification NGSD	action=KEEP	classes=4,5";
	FilterCascade cascade = FilterCascade::fromText(filters);
	FilterResult cascade_result = cascade.apply(variants);

	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << -1.0;
			output.score_explanations << QStringList();
			continue;
		}

		//skip blacklist variants
		const Variant& v = variants[i];
		if (parameters.use_blacklist && blacklist.contains(v))
		{
			output.scores << -2.0;
			output.score_explanations << QStringList();
			continue;
		}

		//init
		CategorizedScores scores;

		//rarity: gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			scores.add("gnomAD", 1.0);
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001)
			{
				scores.add("gnomAD", 0.5);
			}
		}

		//rarity: NGSD count
		QByteArray ngsd_het = v.annotations()[i_ngsd_het].trimmed();
		bool ok = false;
		double ngsd_het2 = ngsd_het.toInt(&ok);
		if (ok && ngsd_het2<=2)
		{
			scores.add("NGSD", 1.0);
		}
		else if (ok && ngsd_het2<=5)
		{
			scores.add("NGSD", 0.5);
		}

		//disease association: in phenotype ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			scores.add("HPO", 2.0);
		}

		//disease association: HGMD variant
		if (i_hgmd!=-1) //optional because of license
		{
			double hgmd_score = 0.0;
			QByteArrayList hgmd = v.annotations()[i_hgmd].trimmed().split(';');
			foreach(const QByteArray& entry, hgmd)
			{
				if (entry.contains("DM?"))
				{
					hgmd_score = std::max(hgmd_score, 0.3);
				}
				else if (entry.contains("DM"))
				{
					hgmd_score = std::max(hgmd_score, 0.5);
				}

			}
			if (hgmd_score>0)
			{
				scores.add("HGMD", hgmd_score);
			}
		}

		//disease association: ClinVar variant
		double clinvar_score = 0.0;
		QByteArrayList clinvar = v.annotations()[i_clinvar].trimmed().split(';');
		foreach(const QByteArray& entry, clinvar)
		{
			if (entry.contains("likely pathogenic"))
			{
				clinvar_score = std::max(clinvar_score, 0.5);
			}
			else if (entry.contains("pathogenic"))
			{
				clinvar_score = std::max(clinvar_score, 1.0);
			}

		}
		if (clinvar_score>0)
		{
			scores.add("ClinVar", clinvar_score);
		}

		//disease association: NGSD classification
		if (parameters.use_ngsd_classifications)
		{
			QByteArray classification = v.annotations()[i_classification].trimmed();
			if (classification=="4")
			{
				scores.add("NGSD class", 0.5);
			}
			if (classification=="5")
			{
				scores.add("NGSD class", 1.0);
			}
		}

		//disease association: OMIM (gene-specific) - format: 612316_[GENE=ATAD3A_PHENOS=...]&616101_[GENE=TMEM240_PHENOS=...]
		if (i_omim!=-1) //optional because of license
		{
			QByteArray omim = v.annotations()[i_omim].trimmed();
			if (!omim.isEmpty())
			{
				QByteArrayList entries = omim.split('&');
				foreach(QByteArray entry, entries)
				{
					QByteArrayList parts = entry.replace("GENE=", "|").replace("_PHENOS=", "|").split('|');
					if (parts.count()<3)
					{
						qDebug() << v.toString() << ": Invalid OMIM entry " << omim;
						continue;
					}
					QByteArray gene = parts[1].trimmed();
					scores.add("OMIM", 1.0, gene);
				}
			}
		}

		//impact (gene-specific)
		QList<VariantTranscript> transcript_info = v.transcriptAnnotations(i_coding);
		foreach(const VariantTranscript& transcript, transcript_info)
		{
			if(transcript.impact=="HIGH")
			{
				scores.add("impact", 3.0, transcript.gene);
			}
			else if(transcript.impact=="MODERATE")
			{
				scores.add("impact", 2.0, transcript.gene);
			}
			else if(transcript.impact=="LOW")
			{
				scores.add("impact", 1.0, transcript.gene);
			}
		}

		//gnomAD o/e lof, inheritance (gene-specific) - format: SAMD11 (inh=n/a oe_syn=1.70 oe_mis=1.51 oe_lof=0.90), NOC2L (inh=n/a oe_syn=1.60 oe_mis=1.25 oe_lof=1.03)
		foreach(QByteArray gene_info, v.annotations()[i_gene_info].split(','))
		{
			gene_info = gene_info.trimmed();
			if (gene_info.isEmpty()) continue;

			//remove bracket at end
			gene_info.chop(1);

			int start = gene_info.indexOf('(');
			if (start==-1)
			{
				qDebug() << v.toString() << ": Invalid gene info " << v.annotations()[i_gene_info];
				continue;
			}

			QByteArray gene = gene_info.left(start-1).trimmed();
			QByteArrayList entries = gene_info.mid(start+1).split(' ');
			foreach(const QByteArray& entry, entries)
			{
				if (entry.startsWith("inh="))
				{
					QByteArray mode = entry.split('=')[1].trimmed();

					if (mode.contains("AD") || mode.contains("XLD"))
					{
						scores.add("gene_inheritance", 0.5, gene);
					}
				}
				if (entry.startsWith("oe_lof="))
				{
					QByteArray oe = entry.split('=')[1].trimmed();
					if (oe!="n/a" && !oe.isEmpty())
					{
						double oe_lof = Helper::toDouble(oe, "gnomAD o/e LOF");
						if (oe_lof<0.1)
						{
							scores.add("gene_oe_lof", 0.5, gene);
						}
					}
				}
			}
		}

		QByteArrayList best_genes;
		output.scores << scores.score(best_genes);
		output.score_explanations << scores.explainations(best_genes);
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_v2_recessive(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const Parameters& parameters)
{
	Result output;

	//get indices of annotations we need
	int i_coding = variants.annotationIndexByName("coding_and_splicing");
	int i_gnomad = variants.annotationIndexByName("gnomAD");
	int i_omim = variants.annotationIndexByName("OMIM", true, false);
	int i_hgmd = variants.annotationIndexByName("HGMD", true, false);
	int i_clinvar = variants.annotationIndexByName("ClinVar");
	int i_gene_info = variants.annotationIndexByName("gene_info");
	int i_classification = variants.annotationIndexByName("classification");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "VariantScores: Algorihtm 'GSvar_v1' can only be applied to variant lists with exactly one affected patient!");
	int i_genotye = affected_cols[0];

	//get blacklist variants
	QList<Variant> blacklist = loadBlacklist();

	//prepare ROI for fast lookup
	if (phenotype_rois.count()==0) output.warnings << "No phenotype region(s) set!";
	BedFile roi;
	foreach(const BedFile& pheno_roi, phenotype_rois)
	{
		roi.add(pheno_roi);
	}
	roi.merge();
	ChromosomalIndex<BedFile> roi_index(roi);

	//apply pre-filters to reduce runtime
	QStringList filters;
	filters << "Allele frequency	max_af=0.1"
			<< "Allele frequency (sub-populations)	max_af=0.1"
			<< "Variant quality	qual=30	depth=1	mapq=20	strand_bias=-1	allele_balance=-1	min_occurences=0	min_af=0	max_af=1"
			<< "Count NGSD	max_count=10	ignore_genotype=false	mosaic_as_het=false"
			<< "Impact	impact=HIGH,MODERATE,LOW"
			<< "Annotated pathogenic	action=KEEP	sources=HGMD,ClinVar	also_likely_pathogenic=false"
			<< "Splice effect	MaxEntScan=LOW	SpliceAi=0.5	action=KEEP"
			<< "Allele frequency	max_af=1.0"
			<< "Filter columns	entries=mosaic	action=REMOVE"
			<< "Classification NGSD	action=REMOVE	classes=1,2";
	if (parameters.use_ngsd_classifications) filters << "Classification NGSD	action=KEEP	classes=4,5";
	FilterCascade cascade = FilterCascade::fromText(filters);
	FilterResult cascade_result = cascade.apply(variants);

	//determine number of hits per gene
	QHash<QString, int> gene_hits_het;
	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i)) continue;

		//skip blacklist variants
		const Variant& v = variants[i];
		if (parameters.use_blacklist && blacklist.contains(v)) continue;

		//skip non-heterozygous variants
		QByteArray v_genotype = v.annotations()[i_genotye].trimmed();
		if (v_genotype!="het") continue;

		//determine gene set (there are typically several transcripts per gene)
		GeneSet genes;
		foreach(const VariantTranscript& transcript, v.transcriptAnnotations(i_coding))
		{
			genes << transcript.gene;
		}
		foreach(const QByteArray& gene, genes)
		{
			gene_hits_het[gene] += 1;
		}
	}

	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << -1.0;
			output.score_explanations << QStringList();
			continue;
		}

		//skip blacklist variants
		const Variant& v = variants[i];
		if (parameters.use_blacklist && blacklist.contains(v))
		{
			output.scores << -2.0;
			output.score_explanations << QStringList();
			continue;
		}

		//init
		CategorizedScores scores;

		//rarity: gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			scores.add("gnomAD", 1.0);
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001)
			{
				scores.add("gnomAD", 0.5);
			}
		}

		//disease association: in phenotype ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			scores.add("HPO", 2.0);
		}

		//disease association: HGMD variant
		if (i_hgmd!=-1) //optional because of license
		{
			double hgmd_score = 0.0;
			QByteArrayList hgmd = v.annotations()[i_hgmd].trimmed().split(';');
			foreach(const QByteArray& entry, hgmd)
			{
				if (entry.contains("DM?"))
				{
					hgmd_score = std::max(hgmd_score, 0.3);
				}
				else if (entry.contains("DM"))
				{
					hgmd_score = std::max(hgmd_score, 0.5);
				}

			}
			if (hgmd_score>0)
			{
				scores.add("HGMD", hgmd_score);
			}
		}

		//disease association: ClinVar variant
		double clinvar_score = 0.0;
		QByteArrayList clinvar = v.annotations()[i_clinvar].trimmed().split(';');
		foreach(const QByteArray& entry, clinvar)
		{
			if (entry.contains("likely pathogenic"))
			{
				clinvar_score = std::max(clinvar_score, 0.5);
			}
			else if (entry.contains("pathogenic"))
			{
				clinvar_score = std::max(clinvar_score, 1.0);
			}

		}
		if (clinvar_score>0)
		{
			scores.add("ClinVar", clinvar_score);
		}

		//disease association: NGSD classification
		if (parameters.use_ngsd_classifications)
		{
			QByteArray classification = v.annotations()[i_classification].trimmed();
			if (classification=="4")
			{
				scores.add("NGSD class", 0.5);
			}
			if (classification=="5")
			{
				scores.add("NGSD class", 1.0);
			}
		}

		//disease association: OMIM (gene-specific) - format: 612316_[GENE=ATAD3A_PHENOS=...]&616101_[GENE=TMEM240_PHENOS=...]
		if (i_omim!=-1) //optional because of license
		{
			QByteArray omim = v.annotations()[i_omim].trimmed();
			if (!omim.isEmpty())
			{
				QByteArrayList entries = omim.split('&');
				foreach(QByteArray entry, entries)
				{
					QByteArrayList parts = entry.replace("GENE=", "|").replace("_PHENOS=", "|").split('|');
					if (parts.count()<3)
					{
						qDebug() << v.toString() << ": Invalid OMIM entry " << omim;
						continue;
					}
					QByteArray gene = parts[1].trimmed();
					scores.add("OMIM", 1.0, gene);
				}
			}
		}

		//impact (gene-specific)
		QList<VariantTranscript> transcript_info = v.transcriptAnnotations(i_coding);
		foreach(const VariantTranscript& transcript, transcript_info)
		{
			if(transcript.impact=="HIGH")
			{
				scores.add("impact", 3.0, transcript.gene);
			}
			else if(transcript.impact=="MODERATE")
			{
				scores.add("impact", 2.0, transcript.gene);
			}
			else if(transcript.impact=="LOW")
			{
				scores.add("impact", 1.0, transcript.gene);
			}
		}

		//gnomAD o/e lof, inheritance (gene-specific) - format: SAMD11 (inh=n/a oe_syn=1.70 oe_mis=1.51 oe_lof=0.90), NOC2L (inh=n/a oe_syn=1.60 oe_mis=1.25 oe_lof=1.03)
		QByteArray v_genotype = v.annotations()[i_genotye].trimmed();
		foreach(QByteArray gene_info, v.annotations()[i_gene_info].split(','))
		{
			gene_info = gene_info.trimmed();
			if (gene_info.isEmpty()) continue;

			//remove bracket at end
			gene_info.chop(1);

			int start = gene_info.indexOf('(');
			if (start==-1)
			{
				qDebug() << v.toString() << ": Invalid gene info " << v.annotations()[i_gene_info];
				continue;
			}

			QByteArray gene = gene_info.left(start-1).trimmed();
			QByteArrayList entries = gene_info.mid(start+1).split(' ');
			foreach(const QByteArray& entry, entries)
			{
				if (entry.startsWith("inh="))
				{
					QByteArray mode = entry.split('=')[1].trimmed();

					if (mode.contains("AR") || mode.contains("XLR"))
					{
						scores.add("gene_inheritance", 0.5, gene);
					}
				}
				if (entry.startsWith("oe_lof="))
				{
					QByteArray oe = entry.split('=')[1].trimmed();
					if (oe!="n/a" && !oe.isEmpty())
					{
						double oe_lof = Helper::toDouble(oe, "gnomAD o/e LOF");
						if (oe_lof<0.1)
						{
							scores.add("gene_oe_lof", 0.5, gene);
						}
					}
				}
			}
		}

		//genotype (gene-specific for heterozygous variants)
		if (v_genotype=="hom")
		{
			scores.add("genotype_hom", 1.0);
		}
		if (v_genotype=="het")
		{
			foreach(const VariantTranscript& transcript, transcript_info)
			{
				const QByteArray& gene = transcript.gene;
				if(gene_hits_het.value(gene, 0)>=2)
				{
					scores.add("genotype_comp_het", 1.0, gene);
				}
			}
		}

		QByteArrayList best_genes;
		output.scores << scores.score(best_genes);
		output.score_explanations << scores.explainations(best_genes);
	}


	return output;
}

//Performance history DOMINANT										Samples / Rank1  / Top10
//version 1															?       / 76.82% / 97.67% (19.04.23)
//score by gene, NGSD score, fix of OE/inheritance parsing			?       / 81.72% / 97.51% (19.04.23)
//more data, pre-filtering of cases by variant genotype				1283    / 81.37% / 97.51% (05.07.23)
//prefilter variants by class 4/5									1278    / 81.69% / 97.50% (05.07.23)
//removing extend 5000 of ROI										1278    / 81.92% / 97.50% (06.07.23)

//Performance history RECESSIVE - HOMOYZGOUOS						Samples / Top1   / Top10
//more data, pre-filtering of cases by variant genotype				553     / 76.13% / 94.58% (05.07.23)
//prefilter variants by class 4/5									549     / 76.68% / 94.72% (05.07.23)
//removing extend 5000 of ROI										549     / 76.87% / 94.72% (06.07.23)

//Performance history RECESSIVE - COMP-HET							Samples / Top2   / Top10
//more data, pre-filtering of cases by variant genotype				775     / 81.94% / 95.35% (05.07.23)
//prefilter variants by class 4/5									770     / 83.77% / 95.84% (05.07.23)
//removing extend 5000 of ROI										770     / 84.03% / 96.10% (06.07.23)

//TODO: Ideas
// - score only relevant transcripts OR score them higher than other transcripts
// - check if recurring variants are a problem > blacklist
// - check missed variants - why were they filtered out?
// - test effect of "mosaic_as_het" in NGSD count filter (needs re-annotation and re-caluclation of pre-filtered files)
// - integrate conservedness (phyloP)?
// - additional score if OMIM/ClinVar/HGMD also match HPO terms
// - HPO: add bonus score if more than one phenotype matches OR using Germans model (email 09.12.2020)
//Ideas if we want to publish it separately:
// - optimize scores by machine learning
// - benchmark with existing tools:
//   - create and use version of ClinVar without our commits (we submit to ClinVar)
//   - https://www.cell.com/trends/genetics/fulltext/S0168-9525(22)00179-2
//   - https://academic.oup.com/bib/article/23/2/bbac019/6521702

