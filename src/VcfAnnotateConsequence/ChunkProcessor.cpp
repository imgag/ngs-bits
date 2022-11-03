#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "ToolBase.h"
#include <zlib.h>
#include <QFileInfo>
#include "Helper.h"
#include "VcfFile.h"
#include "VariantList.h"
#include <QMutex>


ChunkProcessor::ChunkProcessor(AnalysisJob &job, const MetaData& settings, const Parameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, settings_(settings)
	, params_(params)
	, reference_(settings.reference)
	, hgvs_anno_(reference_, settings_.max_dist_to_trans, settings_.splice_region_ex, settings_.splice_region_in_5, settings_.splice_region_in_3)
{
	if (params_.debug) QTextStream(stdout) << "ChunkProcessor(): " << job_.index << endl;
}

ChunkProcessor::~ChunkProcessor()
{
	if (params_.debug) QTextStream(stdout) << "~ChunkProcessor(): " << job_.index << endl;
}

// single chunks are processed
void ChunkProcessor::run()
{
	ChromosomalIndex<TranscriptList> transcript_index(settings_.transcripts);
	if (params_.debug) QTextStream(stdout) << "ChunkProcessor::run() " << job_.index << endl;
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
					lines_new.append("##INFO=<ID=" + settings_.tag + ",Number=.,Type=String,Description=\"Consequence annotations from VcfAnnotateConsequence. Format: Allele|Consequence|IMPACT|SYMBOL|HGNC_ID|Feature|Feature_type|BIOTYPE|EXON|INTRON|HGVSc|HGVSp\">\n");
				}
				lines_new.append(line + "\n");
				continue;
			}

			//get annotation data
			lines_new << annotateVcfLine(line, transcript_index);
		}

		job_.lines = lines_new;
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

	Chromosome chr = parts[0];
	int pos = Helper::toInt(parts[1], "VCF position");
	Sequence ref = parts[3].toUpper();
	Sequence alt = parts[4].toUpper();

	//write out multi-allelic and structural variants (e.g. <DEL>) without CSQ annotation
	QVector<Sequence> alts;
	foreach(const QByteArray& alt_part, alt.split(','))
	{
		alts << alt_part;
	}
	if(!VcfLine(chr, pos, ref, alts).isValid())
	{
		++lines_skipped_;
		return line;
	}
	++lines_annotated_;

	//get all transcripts where the variant is completely contained in the region
	int region_start = std::max(pos - settings_.max_dist_to_trans, 0);
	int region_end = pos + ref.length() + settings_.max_dist_to_trans;

	QVector<int> indices = transcript_index.matchingIndices(chr, region_start, region_end-1);

	QByteArrayList consequences;

	//no transcripts in proximity: intergenic variant
	if(indices.isEmpty())
	{
		//treat each alternative allele of multi-allelic variants as a new variant
		foreach(const Sequence& alt_part, alt.split(','))
		{
			VcfLine var_for_anno = VcfLine(chr, pos, ref, QVector<Sequence>() << alt_part);
			VariantConsequence hgvs;
			if(var_for_anno.isSNV())
			{
				hgvs.allele = var_for_anno.alt(0);
			}
			else if(var_for_anno.isDel())
			{
				hgvs.allele = "-";
			}
			else if(var_for_anno.alt(0).at(0) == var_for_anno.ref().at(0))
			{
				hgvs.allele = var_for_anno.alt(0).mid(1);
			}
			else
			{
				hgvs.allele = var_for_anno.alt(0);
			}
			hgvs.types.insert(VariantConsequenceType::INTERGENIC_VARIANT);
			hgvs.impact = VariantHgvsAnnotator::consequenceTypeToImpact(VariantConsequenceType::INTERGENIC_VARIANT);
			Transcript t;
			consequences << hgvsNomenclatureToString(hgvs, t);
		}
	}

	//hgvs annotation for each transcript in proximity to variant
	foreach(int idx, indices)
	{
		const Transcript& t = settings_.transcripts.at(idx);

		//treat each alternative allele of multi-allelic variants as a new variant
		foreach(const Sequence& alt, alt.split(','))
		{
			//create new VcfLine for annotation (don't change original variant coordinates by normalization!)
			VcfLine var_for_anno = VcfLine(chr, pos, ref, QVector<Sequence>() << alt);

			try
			{
				VariantConsequence hgvs = hgvs_anno_.annotate(t, var_for_anno);
				consequences << hgvsNomenclatureToString(hgvs, t);
			}
			catch(ArgumentException& e)
			{
				QTextStream out(stdout);
				out << e.message() << endl;
				out << "Variant out of region for transcript " << t.name() <<": chromosome=" << t.chr().str()  << " start=" << t.start() << " end=" << t.end() << endl;
				out << "Variant chr=" << t.chr().str() << " start=" << pos << endl;
				out << "Variant shifted: start:" << var_for_anno.start() << " end: " << var_for_anno.end() << endl;
				out << "Considered region: " << region_start << " - " << region_end << endl;
			}
		}
	}


	//add CSQ to INFO; keep all other infos
	QByteArrayList info_entries;
	if(parts[7] != "" && parts[7] != ".")
	{
		info_entries = parts[7].split(';');
	}
	//replace entry if tag is already present
	bool tag_found = false;
	for(int i=0; i<info_entries.count(); ++i)
	{
		if(info_entries[i].startsWith(settings_.tag + "="))
		{
			info_entries[i] = settings_.tag + "=" + consequences.join(',');
			tag_found = true;
			break;
		}
	}
	//append tag if not present
	if(!tag_found)
	{
		info_entries << settings_.tag + "=" + consequences.join(',');
	}

	//create annotated line:
	QByteArrayList new_parts;
	new_parts.append(parts[0]);
	new_parts.append(QByteArray::number(pos));
	new_parts.append(parts[2]);
	new_parts.append(ref);
	new_parts.append(alt);
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

QByteArray ChunkProcessor::hgvsNomenclatureToString(const VariantConsequence& hgvs, const Transcript& t)
{
	QByteArrayList output;
	output << hgvs.allele;

	//find variant consequence type with highest priority (apart from splice region)
	VariantConsequenceType max_csq_type = VariantConsequenceType::INTERGENIC_VARIANT;
	foreach(VariantConsequenceType csq_type, hgvs.types)
	{
		if(static_cast<int>(csq_type) > static_cast<int>(max_csq_type) && csq_type != VariantConsequenceType::SPLICE_REGION_VARIANT &&
				csq_type != VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT &&
				csq_type != VariantConsequenceType::SPLICE_DONOR_VARIANT &&
				csq_type != VariantConsequenceType::NMD_TRANSCRIPT_VARIANT)
		{
			max_csq_type = csq_type;
		}
	}

	QByteArray consequence_type = VariantConsequence::typeToString(max_csq_type);
	QByteArray impact = hgvs.impact;

	//additionally insert splice region consequence type (if present) and order types by impact
	if(hgvs.types.contains(VariantConsequenceType::SPLICE_REGION_VARIANT))
	{
		VariantConsequenceType splice_type;
		if(hgvs.types.contains(VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT))
		{
			splice_type = VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT;
		}
		else if(hgvs.types.contains(VariantConsequenceType::SPLICE_DONOR_VARIANT))
		{
			splice_type = VariantConsequenceType::SPLICE_DONOR_VARIANT;
		}
		else
		{
			splice_type = VariantConsequenceType::SPLICE_REGION_VARIANT;
		}

		if(max_csq_type == VariantConsequenceType::INTRON_VARIANT && splice_type != VariantConsequenceType::SPLICE_REGION_VARIANT && splice_type != VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT)
		{
			consequence_type = VariantConsequence::typeToString(splice_type);
			impact = VariantHgvsAnnotator::consequenceTypeToImpact(splice_type);
		}
		else if(splice_type > max_csq_type)
		{
			consequence_type.prepend(VariantConsequence::typeToString(splice_type) + "&");
			impact = VariantHgvsAnnotator::consequenceTypeToImpact(splice_type);
		}
		else
		{
			consequence_type.append("&" + VariantConsequence::typeToString(splice_type));
		}
	}
	if (hgvs.types.contains(VariantConsequenceType::NMD_TRANSCRIPT_VARIANT))
	{
		consequence_type.append("&" + VariantConsequence::typeToString(VariantConsequenceType::NMD_TRANSCRIPT_VARIANT));
	}
	else if (hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT) && !hgvs.types.contains(VariantConsequenceType::NON_CODING_TRANSCRIPT_EXON_VARIANT))
	{
		consequence_type.append("&" + VariantConsequence::typeToString(VariantConsequenceType::NON_CODING_TRANSCRIPT_VARIANT));
	}

//	if (max_csq_type == VariantConsequenceType::STOP_GAINED && hgvs.types.contains(VariantConsequenceType::STOP_GAINED))
//	{
//		consequence_type.append("&" + VariantConsequence::typeToString(VariantConsequenceType::STOP_GAINED));
//	}

	output << consequence_type;

	//add variant impact
	output << impact;

	//gene symbol, HGNC ID, transcript ID, feature type
	if (t.isValid())
	{
		output << t.gene() << t.hgncId() << t.name() + "." + QByteArray::number(t.version()) << "Transcript";
	}
	else
	{
		output << "" << "" << "" << "";
	}

	//biotype
	if(t.isCoding()) output << "protein_coding";
	else if(t.isValid()) output << "processed_transcript";
	else output << "";

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


