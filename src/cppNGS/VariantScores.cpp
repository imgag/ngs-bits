#include "VariantScores.h"
#include "Exceptions.h"
#include "Helper.h"
#include "GeneSet.h"
#include "FilterCascade.h"

VariantScores::VariantScores()
	: algorithms_()
{
	algorithms_ << "GSvar_v1";
	algorithms_ << "GSvar_v1_noNGSD";
}

QStringList VariantScores::algorithms()
{
	return algorithms_;
}

QString VariantScores::description(QString algorithm) const
{
	if (!algorithms_.contains(algorithm))
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

VariantScores::Result VariantScores::score(QString algorithm, const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois) const
{
	if (!algorithms_.contains(algorithm))
	{
		THROW(ArgumentException, "VariantScores: Unregistered algorithm name '" + algorithm + "'!");
	}

	if (algorithm=="GSvar_v1")
	{
		return score_GSvar_V1(variants, phenotype_rois);
	}
	if (algorithm=="GSvar_v1_noNGSD")
	{
		return score_GSvar_V1_noNGSD(variants, phenotype_rois);
	}

	THROW(ArgumentException, "VariantScores: Not implemented algorithm '" + algorithm + "'!");
}

VariantScores::Result VariantScores::score_GSvar_V1(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois) const
{
	Result output;

	//get indices of annotations we need
	int i_coding = variants.annotationIndexByName("coding_and_splicing");
	int i_gnomad = variants.annotationIndexByName("gnomAD");
	int i_omim = variants.annotationIndexByName("OMIM");
	int i_hgmd = variants.annotationIndexByName("HGMD");
	int i_clinvar = variants.annotationIndexByName("ClinVar");
	int i_gene_info = variants.annotationIndexByName("gene_info");
	int i_classification = variants.annotationIndexByName("classification");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "VariantScores: Algorihtm 'GSvar_V1' can only be applied to variant lists with exactly one affected patient!");
	int i_genotye = affected_cols[0];

	//prepare ROI for fast lookup
	//Possible imrovement: Count each phenotype ROI hit separatly?!
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
		"Classification NGSD	action=REMOVE	classes=1,2" <<
		"Classification NGSD	action=KEEP	classes=4,5");

	FilterResult cascade_result = cascade.apply(variants);

	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << 0.0;
			continue;
		}

		const Variant& v = variants[i];
		bool debug = false;

		//get gene/transcript list
		QList<VariantTranscript> transcript_info = Variant::parseTranscriptString(v.annotations()[i_coding]);
		GeneSet genes;
		foreach(const VariantTranscript& transcript, transcript_info)
		{
			genes << transcript.gene;
		}

		double score = 0.0;

		//in phenotye ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			score += 2.0;
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
		score += impact_score;
		if (debug) qDebug() << "impact: " << score;

		//gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			score += 1.0;
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001) score += 0.5;
		}
		if (debug) qDebug() << "af_gnomad: " << score;

		//OMIM gene
		QByteArray omim = v.annotations()[i_omim].trimmed();
		if (!omim.isEmpty()) score += 1.0;
		if (debug) qDebug() << "OMIM: " << score;

		//HGMD
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
		score += hgmd_score;
		if (debug) qDebug() << "HGMD: " << score;

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
		score += clinvar_score;
		if (debug) qDebug() << "ClinVar: " << score;

		//NGSD classification
		QByteArray classification = v.annotations()[i_classification].trimmed();
		if (classification=="4") score += 0.5;
		if (classification=="5") score += 1;
		if (debug) qDebug() << "classification: " << score;


		//gene-specific infos (gnomAD o/e lof, inheritance)
		bool inh_match = false;
		double min_oe = 1;
		QByteArray genotype = v.annotations()[i_genotye].trimmed();
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
		if (inh_match) score += 0.5;
		if (debug) qDebug() << "inh_match: " << score;
		if (min_oe<0.1) score += 0.5;
		if (debug) qDebug() << "min_oe: " << score;

		output.scores << score;
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_V1_noNGSD(const VariantList& variants, QHash<Phenotype, BedFile> phenotype_rois) const
{
	Result output;

	//get indices of annotations we need
	int i_coding = variants.annotationIndexByName("coding_and_splicing");
	int i_gnomad = variants.annotationIndexByName("gnomAD");
	int i_omim = variants.annotationIndexByName("OMIM");
	int i_hgmd = variants.annotationIndexByName("HGMD");
	int i_clinvar = variants.annotationIndexByName("ClinVar");
	int i_gene_info = variants.annotationIndexByName("gene_info");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "VariantScores: Algorihtm 'GSvar_V1' can only be applied to variant lists with exactly one affected patient!");
	int i_genotye = affected_cols[0];

	//prepare ROI for fast lookup
	//Possible imrovement: Count each phenotype ROI hit separatly?!
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
			output.scores << 0.0;
			continue;
		}

		const Variant& v = variants[i];
		bool debug = false;

		//get gene/transcript list
		QList<VariantTranscript> transcript_info = Variant::parseTranscriptString(v.annotations()[i_coding]);
		GeneSet genes;
		foreach(const VariantTranscript& transcript, transcript_info)
		{
			genes << transcript.gene;
		}

		double score = 0.0;

		//in phenotye ROI
		int index = roi_index.matchingIndex(v.chr(), v.start(), v.end());
		if (index!=-1)
		{
			score += 2.0;
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
		score += impact_score;
		if (debug) qDebug() << "impact: " << score;

		//gnomAD
		QByteArray af_gnomad = v.annotations()[i_gnomad].trimmed();
		if (af_gnomad=="")
		{
			score += 1.0;
		}
		else
		{
			double af_gnomad2 = Helper::toDouble(af_gnomad, "genomAD AF");
			if (af_gnomad2<=0.0001) score += 0.5;
		}
		if (debug) qDebug() << "af_gnomad: " << score;

		//OMIM gene
		QByteArray omim = v.annotations()[i_omim].trimmed();
		if (!omim.isEmpty()) score += 1.0;
		if (debug) qDebug() << "OMIM: " << score;

		//HGMD
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
		score += hgmd_score;
		if (debug) qDebug() << "HGMD: " << score;

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
		score += clinvar_score;
		if (debug) qDebug() << "ClinVar: " << score;

		//gene-specific infos (gnomAD o/e lof, inheritance)
		bool inh_match = false;
		double min_oe = 1;
		QByteArray genotype = v.annotations()[i_genotye].trimmed();
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
		if (inh_match) score += 0.5;
		if (debug) qDebug() << "inh_match: " << score;
		if (min_oe<0.1) score += 0.5;
		if (debug) qDebug() << "min_oe: " << score;

		output.scores << score;
	}

	return output;
}
