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
		algorithms << "GSvar_v1_noNGSD";
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
	if (algorithm=="GSvar_v1_noNGSD")
	{
		return "Variant ranking based on clinical information only (without the use for NGSD classifications - mainly for testing)";
	}

	THROW(ArgumentException, "VariantScores::description: Not implemented algorithm '" + algorithm + "'!");
}

VariantScores::Result VariantScores::score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const QList<Variant>& blacklist)
{
	if (!algorithms().contains(algorithm))
	{
		THROW(ArgumentException, "VariantScores: Unregistered algorithm name '" + algorithm + "'!");
	}

	//score
	VariantScores::Result result;
	if (algorithm=="GSvar_v1")
	{
		 result = score_GSvar_V1(variants, phenotype_rois, blacklist);
	}
	else if (algorithm=="GSvar_v1_noNGSD")
	{
		result = score_GSvar_V1_noNGSD(variants, phenotype_rois, blacklist);
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

int VariantScores::annotate(VariantList& variants, const VariantScores::Result& result, bool add_explainations)
{
	//check input
	if (variants.count()!=result.scores.count()) THROW(ProgrammingException, "Variant list and scoring result differ in count!");

	//add columns if missing
	if (add_explainations && variants.annotationIndexByName("GSvar_score_explainations", true, false)==-1)
	{
		variants.prependAnnotation("GSvar_score_explainations", "GSvar score explainations.");
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
	int i_score_exp = -1;
	if (add_explainations)
	{
		i_score_exp = variants.annotationIndexByName("GSvar_score_explainations");
	}

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
		if (add_explainations)
		{
			variants[i].annotations()[i_score_exp] = result.score_explainations[i].join(", ").toUtf8();
		}
	}

	return c_scored;
}

QList<Variant> VariantScores::blacklist()
{
	QList<Variant> output;

	QStringList entries = Settings::stringList("ranking_variant_blacklist", true);
	foreach(QString entry, entries)
	{
		output << Variant::fromString(entry);
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_V1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const QList<Variant>& blacklist)
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
	FilterCascade cascade = FilterCascade::fromText(QStringList() <<
		"Allele frequency	max_af=0.1" <<
		"Allele frequency (sub-populations)	max_af=0.1" <<
		"Variant quality	qual=30	depth=5	mapq=20	strand_bias=-1	allele_balance=-1" <<
		"Count NGSD	max_count=10	ignore_genotype=false" <<
		"Impact	impact=HIGH,MODERATE,LOW" <<
		"Annotated pathogenic	action=KEEP	sources=HGMD,ClinVar	also_likely_pathogenic=false" <<
		"Classification NGSD	action=KEEP	classes=4,5" <<
		"Allele frequency	max_af=1.0" <<
		"Classification NGSD	action=REMOVE	classes=1,2");

	FilterResult cascade_result = cascade.apply(variants);

	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << -1.0;
			output.score_explainations << QStringList();
			continue;
		}

		//skip blacklist variants
		const Variant& v = variants[i];
		if (blacklist.contains(v))
		{
			output.scores << -1.0;
			output.score_explainations << QStringList();
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
		QStringList explainations;

		//in phenotype ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			score += 2.0;
			explainations << "HPO:2.0";
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
			explainations << "impact:" + QString::number(impact_score, 'f', 1);
		}

		//gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			score += 1.0;
			explainations << "gnomAD:1.0";
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001)
			{
				score += 0.5;
				explainations << "gnomAD:0.5";
			}
		}

		//OMIM gene
		if (i_omim!=-1) //optional because of license
		{
			QByteArray omim = v.annotations()[i_omim].trimmed();
			if (!omim.isEmpty())
			{
				score += 1.0;
				explainations << "OMIM:1.0";
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
				explainations << "HGMD:" + QString::number(hgmd_score, 'f', 1);
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
			explainations << "ClinVar:" + QString::number(clinvar_score, 'f', 1);
		}

		//NGSD classification
		QByteArray classification = v.annotations()[i_classification].trimmed();
		if (classification=="4")
		{
			score += 0.5;
			explainations << "NGSD class:0.5";
		}
		if (classification=="5")
		{
			score += 1;
			explainations << "NGSD class:1.0";
		}

		//genotype
		QByteArray genotype = v.annotations()[i_genotye].trimmed();
		if (genotype=="hom")
		{
			score += 1.0;
			explainations << "homozygous:1.0";
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
			explainations << "gene_inheritance:0.5";
		}
		if (min_oe<0.1)
		{
			score += 0.5;
			explainations << "gene_oe:0.5";
		}

		output.scores << score;
		output.score_explainations << explainations;
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_V1_noNGSD(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois, const QList<Variant>& blacklist)
{
	Result output;

	//get indices of annotations we need
	int i_coding = variants.annotationIndexByName("coding_and_splicing");
	int i_gnomad = variants.annotationIndexByName("gnomAD");
	int i_omim = variants.annotationIndexByName("OMIM", true, false);
	int i_hgmd = variants.annotationIndexByName("HGMD", true, false);
	int i_clinvar = variants.annotationIndexByName("ClinVar");
	int i_gene_info = variants.annotationIndexByName("gene_info");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "VariantScores: Algorihtm 'GSvar_v1_noNGSD' can only be applied to variant lists with exactly one affected patient!");
	int i_genotye = affected_cols[0];

	//prepare ROI for fast lookup
	//TODO: test counting each phenotype ROI hit separatly OR using Germans model (email 09.12.2020)
	if (phenotype_rois.count()==0) output.warnings << "No phenotype region(s) set!";
	BedFile roi;
	foreach(const BedFile& pheno_roi, phenotype_rois)
	{
		roi.add(pheno_roi);
	}
	roi.merge();
	ChromosomalIndex<BedFile> roi_index(roi);

	//apply pre-filters to reduce runtime
	FilterCascade cascade = FilterCascade::fromText(QStringList() <<
		"Allele frequency	max_af=0.1" <<
		"Allele frequency (sub-populations)	max_af=0.1" <<
		"Variant quality	qual=30	depth=5	mapq=20	strand_bias=-1	allele_balance=-1" <<
		"Count NGSD	max_count=10	ignore_genotype=false" <<
		"Impact	impact=HIGH,MODERATE,LOW" <<
		"Annotated pathogenic	action=KEEP	sources=HGMD,ClinVar	also_likely_pathogenic=false" <<
		"Allele frequency	max_af=1.0" <<
		"Classification NGSD	action=REMOVE	classes=1,2");

	FilterResult cascade_result = cascade.apply(variants);

	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << -1.0;
			output.score_explainations << QStringList();
			continue;
		}

		//skip blacklist variants
		const Variant& v = variants[i];
		if (blacklist.contains(v))
		{
			output.scores << -1.0;
			output.score_explainations << QStringList();
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
		QStringList explainations;

		//in phenotype ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			score += 2.0;
			explainations << "HPO:2.0";
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
			explainations << "impact:" + QString::number(impact_score, 'f', 1);
		}

		//gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			score += 1.0;
			explainations << "gnomAD:1.0";
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001)
			{
				score += 0.5;
				explainations << "gnomAD:0.5";
			}
		}

		//OMIM gene
		if (i_omim!=-1) //optional because of license
		{
			QByteArray omim = v.annotations()[i_omim].trimmed();
			if (!omim.isEmpty())
			{
				score += 1.0;
				explainations << "OMIM:1.0";
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
				explainations << "HGMD:" + QString::number(hgmd_score, 'f', 1);
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
			explainations << "ClinVar:" + QString::number(clinvar_score, 'f', 1);
		}

		//genotype
		QByteArray genotype = v.annotations()[i_genotye].trimmed();
		if (genotype=="hom")
		{
			score += 1.0;
			explainations << "homozygous:1.0";
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
			explainations << "gene_inheritance:0.5";
		}
		if (min_oe<0.1)
		{
			score += 0.5;
			explainations << "gene_oe:0.5";
		}

		output.scores << score;
		output.score_explainations << explainations;
	}

	return output;
}
