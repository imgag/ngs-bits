#include "VariantScores.h"
#include "Exceptions.h"
#include "Helper.h"
#include "GeneSet.h"
#include "FilterCascade.h"
#include <math.h>

VariantScores::VariantScores()
{
}

QStringList VariantScores::algorithms()
{
	static QStringList algorithms;
	if (algorithms.isEmpty())
	{
		algorithms << "GSvar_v3";
	}
	return algorithms;
}

QString VariantScores::description(QString algorithm)
{
	if (!algorithms().contains(algorithm))
	{
		THROW(ArgumentException, "VariantScores::description: Unregistered algorithm name '" + algorithm + "'!");
	}

	if (algorithm=="GSvar_v3")
	{
		return "Variant ranking based on clinical evidence.";
	}

	THROW(ArgumentException, "VariantScores::description: Not implemented algorithm '" + algorithm + "'!");
}

VariantScores::Result VariantScores::score(QString algorithm, const VariantList& variants, const QHash<Phenotype, BedFile>& phenotype_rois, const Parameters& parameters)
{
	if (!algorithms().contains(algorithm))
	{
		THROW(ArgumentException, "VariantScores: Unregistered algorithm name '" + algorithm + "'!");
	}

	//score
	VariantScores::Result result;
	if (algorithm=="GSvar_v3")
	{
		result = score_GSvar_v3(variants, phenotype_rois, parameters);
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
	QList<VariantAnnotationDescription> new_annotations;
	QByteArrayList default_values;
	if (variants.annotationIndexByName("GSvar_rank", true, false)==-1)
	{
		new_annotations << VariantAnnotationDescription("GSvar_rank", "GSvar score based rank.");
		default_values << QByteArray();
	}
	if (variants.annotationIndexByName("GSvar_score", true, false)==-1)
	{
		new_annotations << VariantAnnotationDescription("GSvar_score", "GSvar score (algorithm: " + result.algorithm + ", description:" + VariantScores::description(result.algorithm)+  ")");
		default_values << QByteArray();
	}
	if (add_explanations && variants.annotationIndexByName("GSvar_score_explanations", true, false)==-1)
	{
		new_annotations << VariantAnnotationDescription("GSvar_score_explanations", "GSvar score explanations.");
		default_values << QByteArray();
	}
	variants.prependAnnotations(new_annotations, default_values);
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

QByteArrayList VariantScores::prefilters(const Parameters& parameters)
{
	QByteArrayList filters;
	filters << "Allele frequency	max_af=0.1"
			<< "Allele frequency (sub-populations)	max_af=0.1"
			<< "Variant quality	qual=20	depth=1"
			<< "Count NGSD	max_count=10	ignore_genotype=false	mosaic_as_het=false"
			<< "Impact	impact=HIGH,MODERATE,LOW"
			<< "Splice effect	MaxEntScan=LOW	SpliceAi=0.5	splice_site_only=false	action=KEEP"
			<< "Count NGSD	max_count=100	ignore_genotype=false	mosaic_as_het=false" //make sure too common variants and artefacts with splicing effect prediction are not kept
			<< "Annotated pathogenic	action=KEEP	sources=HGMD,ClinVar	also_likely_pathogenic=false"
			<< "Allele frequency	max_af=1.0"
			<< "Filter columns	entries=mosaic	action=REMOVE"; //TODO minus points instead of filtering?
	if (parameters.use_ngsd_classifications)
	{
		filters << "Classification NGSD	action=REMOVE	classes=1,2";
		filters << "Classification NGSD	action=KEEP	classes=4,5";
	}

	return filters;
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
    for (const QByteArray& gene : keys())
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
    for (const QByteArray& gene : best_genes)
	{
		if (best_genes.count()>1) output << "[" + gene + "]";
		output << explainations(gene);
	}

	return output;
}

VariantScores::Result VariantScores::score_GSvar_v3(const VariantList& variants, const QHash<Phenotype, BedFile>& phenotype_rois, const Parameters& parameters)
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
	int i_phylop = variants.annotationIndexByName("phyloP");
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()!=1) THROW(ArgumentException, "Ranking can only be applied to variant lists with exactly one affected patient!");
	int i_genotye = affected_cols[0];

	//prepare ROI for fast lookup
	if (phenotype_rois.count()==0) output.warnings << "No phenotype region(s) set!";
	QHash<Phenotype, QSharedPointer<ChromosomalIndex<BedFile>>> phenotype_rois_indices;
	for(auto it=phenotype_rois.cbegin(); it!=phenotype_rois.cend(); ++it)
	{
		phenotype_rois_indices.insert(it.key(), QSharedPointer<ChromosomalIndex<BedFile>>(new ChromosomalIndex<BedFile>(it.value())));
	}

	//apply pre-filters to reduce runtime
	QByteArrayList filters = prefilters(parameters);
	FilterCascade cascade = FilterCascade::fromText(filters);
	FilterResult cascade_result = cascade.apply(variants);

	//determine number of hits per gene
	QHash<QString, int> gene_hits_het; //TODO use?
	for (int i=0; i<variants.count(); ++i)
	{
		//skip pre-filtered variants
		if(!cascade_result.passing(i)) continue;

		//skip non-heterozygous variants
		const Variant& v = variants[i];
		QByteArray v_genotype = v.annotations()[i_genotye].trimmed();
		if (v_genotype!="het") continue;

		//determine gene set (there are typically several transcripts per gene)
		GeneSet genes;
		for (const VariantTranscript& transcript : v.transcriptAnnotations(i_coding))
		{
			genes << transcript.gene;
		}
		for (const QByteArray& gene : std::as_const(genes))
		{
			gene_hits_het[gene] += 1;
		}
	}

	for (int i=0; i<variants.count(); ++i)
	{
		const Variant& v = variants[i];

		//skip pre-filtered variants
		if(!cascade_result.passing(i))
		{
			output.scores << -1.0;
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
			double af_gnomad2 = Helper::toDouble(af_gnomad, "gnomAD AF");
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
		int pheno_roi_hits = 0;
		for(auto it=phenotype_rois_indices.begin(); it!=phenotype_rois_indices.end(); ++it)
		{
			int index = it.value()->matchingIndex(v.chr(), v.start(), v.end());
			if (index!=-1) ++pheno_roi_hits;
		}
		if (pheno_roi_hits>0)
		{
			double pheno_score = 1.0 + sqrt(pheno_roi_hits);
			pheno_score = truncf(pheno_score * 10.0) / 10.0;
			scores.add("HPO", pheno_score);
		}

		//disease association: HGMD variant
		if (i_hgmd!=-1) //optional because of license
		{
			double hgmd_score = 0.0;
			QByteArrayList hgmd = v.annotations()[i_hgmd].trimmed().split(';');
			for (const QByteArray& entry : std::as_const(hgmd))
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
		for (const QByteArray& entry : std::as_const(clinvar))
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
				for (QByteArray entry : std::as_const(entries))
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

		//disease association: conservedness
		double phylop = v.annotations()[i_phylop].trimmed().toDouble(); //0 if no conversion possible;
		if (phylop>=1.6)
		{
			scores.add("phyloP", 0.3);
		}

		//impact (gene-specific)
		QList<VariantTranscript> transcript_info = v.transcriptAnnotations(i_coding);
		for (const VariantTranscript& transcript : std::as_const(transcript_info))
		{
			if(transcript.impact==VariantImpact::HIGH)
			{
				scores.add("impact", 3.0, transcript.gene);
			}
			else if(transcript.impact==VariantImpact::MODERATE)
			{
				scores.add("impact", 2.0, transcript.gene);
			}
			else if(transcript.impact==VariantImpact::LOW)
			{
				scores.add("impact", 1.0, transcript.gene);
			}
		}

		//gnomAD o/e lof, inheritance (gene-specific) - format: SAMD11 (inh=n/a oe_syn=1.70 oe_mis=1.51 oe_lof=0.90), NOC2L (inh=n/a oe_syn=1.60 oe_mis=1.25 oe_lof=1.03)
		QByteArray v_genotype = v.annotations()[i_genotye].trimmed();
		for (QByteArray gene_info : v.annotations()[i_gene_info].split(','))
		{
			gene_info = gene_info.trimmed();
			if (gene_info.isEmpty()) continue;

			//remove bracket at end
			gene_info.chop(1);

			//skip entry if no info available
			int start = gene_info.indexOf('(');
			if (start==-1) continue;

			QByteArray gene = gene_info.left(start-1).trimmed();
			QByteArrayList entries = gene_info.mid(start+1).split(' ');
			for (const QByteArray& entry : std::as_const(entries))
			{
				if (entry.startsWith("inh="))
				{
					QByteArray mode = entry.split('=')[1].trimmed();

					if (v_genotype=="hom" && (mode.contains("AR") || mode.contains("XLR")))
					{
						scores.add("gene_inheritance", 0.5, gene);
					}
					else if (v_genotype=="het" && (mode.contains("AD") || mode.contains("XLD")))
					{
						scores.add("gene_inheritance", 0.5, gene);
					}
					//TODO comp-het?
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
