#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "Helper.h"
#include "VcfFile.h"
#include "VariantList.h"


struct Var
{
	Chromosome chr;
	int pos;
	Sequence ref;
	Sequence alt;
	bool source;
	QByteArray tag;
};

ChunkProcessor::ChunkProcessor(AnalysisJob &job, const MetaData& settings, const Parameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, settings_(settings)
	, params_(params)
	, reference_(settings.reference)
	, hgvs_anno_(reference_, settings_.annotation_parameters)
{
    if (params_.debug) QTextStream(stdout) << "ChunkProcessor(): " << job_.index << Qt::endl;
}

ChunkProcessor::~ChunkProcessor()
{
    if (params_.debug) QTextStream(stdout) << "~ChunkProcessor(): " << job_.index << Qt::endl;
}

// single chunks are processed
void ChunkProcessor::run()
{
	ChromosomalIndex<TranscriptList> transcript_index(settings_.transcripts);
    if (params_.debug) QTextStream(stdout) << "ChunkProcessor::run() " << job_.index << Qt::endl;
	try
	{
		// read vcf file
		QByteArrayList lines_new;
		foreach(QByteArray line, job_.lines)
		{
			line = line.trimmed();
			//skip empty lines
			if (line.isEmpty()) continue;

			//write out headers unchanged
			if (line.startsWith('#'))
			{
				if (line.startsWith("##INFO=<ID=" + settings_.tag + ","))
				{
					continue;
				}

				//append header line for new annotation
				if (line.startsWith("#CHROM"))
				{
					lines_new.append("##INFO=<ID=" + settings_.tag + ",Number=.,Type=String,Description=\"Consequence annotations from VcfAnnotateConsequence. Format: Allele|Consequence|IMPACT|SYMBOL|HGNC_ID|Feature|Feature_type|EXON|INTRON|HGVSc|HGVSp\">\n");
					if (params_.source != "refseq") lines_new.append("##INFO=<ID=" + settings_.tag + "_SOURCE" + ",Number=.,Type=String,Description=\"Source variant consequence annotations from VcfAnnotateConsequence. Format: Allele|Consequence|IMPACT|SYMBOL|HGNC_ID|Feature|Feature_type|EXON|INTRON|HGVSc|HGVSp\">\n");
				}
				lines_new.append(line + "\n");
				continue;
			}

			//get annotation data
			lines_new << annotateVcfLine(line, transcript_index);
		}

		job_.lines = lines_new;
		emit log(lines_annotated_, lines_skipped_);
		emit done(job_.index);

	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}

QByteArray ChunkProcessor::annotateVcfLine(const QByteArray& line, const ChromosomalIndex<TranscriptList>& transcript_index)
{
	//split line and extract variant infos
	QList<QByteArray> parts = line.split('\t');
	if (parts.count()<VcfFile::MIN_COLS)
	{
		THROW(FileParseException, "VCF line with too few columns: " + line);
	}

	QList<Var> variants;

	if (line.contains("SOURCE_VAR") && params_.source != "refseq")
	{
		QByteArrayList infos = parts[7].split(';');
		foreach (const QByteArray &p, infos)
		{
			if (p.startsWith("SOURCE_VAR"))
			{
				QByteArrayList var_parts = p.split('=')[1].split('&');
				Var source_var;

				source_var.chr = var_parts[0];
				source_var.pos = Helper::toInt(var_parts[1], "VCF position");
				source_var.ref = var_parts[2].toUpper();
				source_var.alt = var_parts[3].toUpper();
				source_var.source = true;
				source_var.tag = settings_.tag + "_SOURCE";

				variants.append(source_var);
			}
		}
	}

	Var processed_var;
	processed_var.chr = parts[0];
	processed_var.pos = Helper::toInt(parts[1], "VCF position");
	processed_var.ref = parts[3].toUpper();
	processed_var.alt = parts[4].toUpper();
	processed_var.source = false;
	processed_var.tag = settings_.tag;

	variants.append(processed_var);

	QByteArrayList info_entries;
	if(parts[7] != "" && parts[7] != ".")
	{
		info_entries = parts[7].split(';');
	}

	foreach (const Var &v, variants)
	{
		//write out invalid without CSQ annotation
		if(!VcfLine(v.chr, v.pos, v.ref, v.alt.split(',')).isValid())
		{
			if (v.source) continue;
			else
			{
				++lines_skipped_;
				return line + "\n";
			}
		}

		//get all transcripts where the variant is completely contained in the region
		int region_start = std::max(v.pos - settings_.annotation_parameters.max_dist_to_transcript, 0);
		int region_end = v.pos + v.ref.length() + settings_.annotation_parameters.max_dist_to_transcript;

		QVector<int> indices = transcript_index.matchingIndices(v.chr, region_start, region_end-1);

		QByteArrayList consequences;

		//no transcripts in proximity: intergenic variant
		if(indices.isEmpty())
		{
			foreach(const Sequence& alt_part, v.alt.split(','))
			{
				VariantConsequence hgvs;
				hgvs.types.insert(VariantConsequenceType::INTERGENIC_VARIANT);
				hgvs.impact = VariantHgvsAnnotator::consequenceTypeToImpact(VariantConsequenceType::INTERGENIC_VARIANT);
				Transcript t;
				consequences << hgvsNomenclatureToString(csqAllele(v.ref, alt_part), hgvs, t);
			}
		}

		//hgvs annotation for each transcript in proximity to variant
		foreach(int idx, indices)
		{
			const Transcript& t = settings_.transcripts.at(idx);

			VcfLine variant = VcfLine(v.chr, v.pos, v.ref, QList<Sequence>());

			//treat each alternative allele of multi-allelic variants as a new variant
			foreach(const Sequence& alt_part, v.alt.split(','))
			{
				try
				{
					variant.setSingleAlt(alt_part);
					VariantConsequence hgvs = hgvs_anno_.annotate(t, variant);
					consequences << hgvsNomenclatureToString(csqAllele(v.ref, alt_part), hgvs, t);
				}
				catch(Exception& e)
				{
					QTextStream out(stdout);
					out << "Error processing variant " << variant.toString() << " and transcript " << t.nameWithVersion() << ":" << Qt::endl;
					out << "  " << e.message().replace("\n", "  \n") << Qt::endl;
				}
			}
		}

		//add CSQ to INFO; keep all other infos
		//replace entry if tag is already present
		bool tag_found = false;
		for(int i=0; i<info_entries.count(); ++i)
		{
			if(info_entries[i].startsWith(v.tag + "="))
			{
				info_entries[i] = v.tag + "=" + consequences.join(',');
				tag_found = true;
				break;
			}
		}
		//append tag if not present
		if(!tag_found)
		{
			info_entries << v.tag + "=" + consequences.join(',');
		}
	}

	//create annotated line:
	++lines_annotated_;
	QByteArrayList new_parts;
	new_parts.append(parts[0]);
	new_parts.append(QByteArray::number(processed_var.pos));
	new_parts.append(parts[2]);
	new_parts.append(processed_var.ref);
	new_parts.append(processed_var.alt);
	new_parts.append(parts[5]);
	new_parts.append(parts[6]);
	new_parts.append(info_entries.join(';'));

	for(int i=8; i<parts.count(); ++i)
	{
		new_parts.append(parts[i]);
	}
	QByteArray new_line = new_parts.join('\t');
	new_line.append("\n");
	return new_line;
}

QByteArray ChunkProcessor::hgvsNomenclatureToString(const QByteArray& allele, const VariantConsequence& hgvs, const Transcript& t)
{
	QByteArrayList output;
	output << allele;
	output << hgvs.typesToStringSimplified();
	output << variantImpactToString(hgvs.impact);

	//gene symbol, HGNC ID, transcript ID, feature type
	if (t.isValid())
	{
		output << t.gene() << t.hgncId() << t.nameWithVersion() << "Transcript";
	}
	else
	{
		output << "" << "" << "" << "";
	}

	//exon number
	if(hgvs.exon_number != -1) output << QByteArray::number(hgvs.exon_number) + "/" + QByteArray::number(t.regions().count());
	else output << "";

	//intron number
	if(hgvs.intron_number != -1) output << QByteArray::number(hgvs.intron_number) + "/" + QByteArray::number(t.regions().count() - 1);
	else output << "";

	//HGVS.c and HGVS.p
	output << hgvs.hgvs_c;
	QByteArray hgvs_p = hgvs.hgvs_p;
	hgvs_p.replace("=", "%3D");
	output << hgvs_p;

	return output.join('|');
}

QByteArray ChunkProcessor::csqAllele(const Sequence& ref, const Sequence& alt)
{
	if (ref.length()==0) THROW(Exception, "Invalid reference sequence for VCF variant: '" + ref + "'");
	if (alt.length()==0) THROW(Exception, "Invalid alternative sequence for VCF variant: '" + alt + "'");

	//deletion
	if(alt.length()==1 && ref.length() > 1)
	{
		return "-";
	}

	//insertions and complex variants
	if(ref[0] == alt[0])
	{
		return alt.mid(1);
	}

	//SNVs and MNPs
	return alt;
}


