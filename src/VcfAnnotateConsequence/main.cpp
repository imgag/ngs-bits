#include "ToolBase.h"
#include "Settings.h"
#include "NGSHelper.h"
#include "Helper.h"
#include "VcfFile.h"
#include "VariantHgvsAnnotator.h"

class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

private:
	QPair<int,int> annotateVcfStream(const QString& in_file, const QString& out_file, const ChromosomalIndex<TranscriptList>& transcript_index,
                           const FastaFileIndex& reference, const int& max_dist_to_trans, const int& splice_region_ex,
						   int splice_region_in_5, int splice_region_in_3, const QByteArray& tag)
    {
		//init
		int c_annotated = 0;
		int c_skipped = 0;
		QRegExp valid_alt_regexp("^[ACGT]+(,[ACGT]+)*$");

        if(in_file != "" && in_file == out_file)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }
        QSharedPointer<QFile> reader = Helper::openFileForReading(in_file, true);
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file, true);

		const TranscriptList& transcripts = transcript_index.container();
		VariantHgvsAnnotator hgvs_anno(reference, max_dist_to_trans, splice_region_ex, splice_region_in_5, splice_region_in_3);

        while(!reader->atEnd())
        {
            QByteArray line = reader->readLine();

            //skip empty lines
            if(line.trimmed().isEmpty()) continue;

			//write out header lines unchanged (unless they are the CSQ description)
            if(line.startsWith("##"))
            {
                if(!line.startsWith("##INFO=<ID=" + tag + ","))
                {
                    writer->write(line);
				}
                continue;
            }

            //column header line
            if(line.startsWith("#"))
            {
				//add the CSQ info line after all other header lines
				writer->write("##INFO=<ID=" + tag + ",Number=.,Type=String,Description=\"Consequence annotations from VcfAnnotateConsequence. Format: Allele|Consequence|IMPACT|SYMBOL|HGNC_ID|Feature|Feature_type|BIOTYPE|EXON|INTRON|HGVSc|HGVSp\">\n");

                writer->write(line);
                continue;
            }

            //split line and extract variant infos
            line = line.trimmed();
            QList<QByteArray> parts = line.split('\t');
            if(parts.count() < 8) THROW(FileParseException, "VCF with too few columns: " + line);

            Chromosome chr = parts[0];
			int pos = Helper::toInt(parts[1], "VCF position");
            Sequence ref = parts[3].toUpper();
            Sequence alt = parts[4].toUpper();

            //write out multi-allelic and structural (e.g. <DEL>) variants without CSQ annotation
			if(!valid_alt_regexp.exactMatch(alt))
            {
                writeLine(writer, parts, pos, ref, alt, parts[7]);
				++c_skipped;
                continue;
            }
			++c_annotated;

            //get all transcripts where the variant is completely contained in the region
			int region_start = std::max(pos + ref.length() - max_dist_to_trans, 0);
			int region_end = pos + max_dist_to_trans;

			QVector<int> indices = transcript_index.matchingIndices(chr, region_start, region_end);

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
						hgvs.allele = "-"; //TODO????
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
					hgvs.impact = "MODIFIER";
                    Transcript t;
					consequences << hgvsNomenclatureToString(hgvs, t);
                }
            }

            //hgvs annotation for each transcript in proximity to variant
            foreach(int idx, indices)
            {
				const Transcript& t = transcripts.at(idx);

                //treat each alternative allele of multi-allelic variants as a new variant
				foreach(const Sequence& alt, alt.split(','))
                {
                    //create new VcfLine for annotation (don't change original variant coordinates by normalization!)
					VcfLine var_for_anno = VcfLine(chr, pos, ref, QVector<Sequence>() << alt);

                    try
                    {
						VariantConsequence hgvs = hgvs_anno.annotate(t, var_for_anno);
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
				if(info_entries[i].startsWith(tag + "="))
                {
					info_entries[i] = tag + "=" + consequences.join(',');
                    tag_found = true;
                    break;
                }
			}
			//append tag if not present
            if(!tag_found)
            {
				info_entries << tag + "=" + consequences.join(',');
            }

			writeLine(writer, parts, pos, ref, alt, info_entries.join(';'));
		}

		return qMakePair(c_annotated, c_skipped);
    }

    void writeLine(QSharedPointer<QFile>& writer, const QList<QByteArray>& parts, int pos, const QByteArray& ref, const QByteArray& alt, const QByteArray& info)
	{
		char tab = '\t';
		writer->write(parts[0]);
		writer->write(&tab, 1);
		writer->write(QByteArray::number(pos));
		writer->write(&tab, 1);
		writer->write(parts[2]);
		writer->write(&tab, 1);
		writer->write(ref);
		writer->write(&tab, 1);
		writer->write(alt);
		writer->write(&tab, 1);
		writer->write(parts[5]);
		writer->write(&tab, 1);
		writer->write(parts[6]);
		writer->write(&tab, 1);
		writer->write(info);
		for(int i=8; i<parts.count(); ++i)
		{
			writer->write(&tab, 1);
			writer->write(parts[i]);
		}
		writer->write("\n");
    }

	QByteArray hgvsNomenclatureToString(const VariantConsequence& hgvs, const Transcript& t)
    {
		QByteArrayList output;
		output << hgvs.allele;

        //find variant consequence type with highest priority (apart from splice region)
        VariantConsequenceType max_csq_type = VariantConsequenceType::INTERGENIC_VARIANT;
		foreach(VariantConsequenceType csq_type, hgvs.types)
        {
            if(csq_type > max_csq_type && csq_type != VariantConsequenceType::SPLICE_REGION_VARIANT &&
                    csq_type != VariantConsequenceType::SPLICE_ACCEPTOR_VARIANT &&
                    csq_type != VariantConsequenceType::SPLICE_DONOR_VARIANT)
            {
                max_csq_type = csq_type;
            }
        }
		QString consequence_type = VariantConsequence::typeToString(max_csq_type);

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

            if(splice_type > max_csq_type)
            {
				consequence_type.prepend(VariantConsequence::typeToString(splice_type) + "&");
            }
            else
            {
				consequence_type.append("&" + VariantConsequence::typeToString(splice_type));
            }
        }

		output << consequence_type.toUtf8();
		output << hgvs.impact;

		//gene symbol, HGNC ID, transcript ID, feature type
		if (t.isValid())
		{
			output << t.gene() << t.hgncId() << t.nameWithVersion() << "Transcript";
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
		QByteArray tmp  = hgvs.hgvs_p;
		tmp.replace("=", "%3D");
		output << tmp;

		return output.join('|');
    }

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
		setDescription("Adds transcript-specific variant consequence annotations to a VCF file.");
		addInfile("in", "Input VCF file to annotate.", false);
		addInfile("gff", "Ensembl-style GFF file with transcripts, e.g. from https://ftp.ensembl.org/pub/release-107/gff3/homo_sapiens/Homo_sapiens.GRCh38.107.chr.gff3.gz.", false);

        //optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addOutfile("out", "Output VCF file annotated with predicted consequences for each variant.", false);
		addFlag("all", "If set, all transcripts are annotated. The default is to skip transcripts not labeled with the 'GENCODE basic' tag.");
		addString("tag", "Info field name used for the consequence annotation.", true, "CSQ");
		addInt("max_dist_to_trans", "Maximum distance between variant and transcript.", true, 5000);
		addInt("splice_region_ex", "Number of bases at exon boundaries that are considered to be part of the splice region.", true, 3);
		addInt("splice_region_in5", "Number of bases at intron boundaries (5') that are considered to be part of the splice region.", true, 20);
		addInt("splice_region_in3", "Number of bases at intron boundaries (3') that are considered to be part of the splice region.", true, 20);
    }

    virtual void main()
    {
		// init
        QString ref_file = getInfile("ref");
        if (ref_file=="") ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
        FastaFileIndex reference(ref_file);

        QString in_file = getInfile("in");
        QString gff_file = getInfile("gff");
        QString out_file = getOutfile("out");
        bool all = getFlag("all");
        QByteArray tag = getString("tag").toUtf8();

		int max_dist_to_trans = getInt("max_dist_to_trans");
		int splice_region_ex = getInt("splice_region_ex");
		int splice_region_in_5 = getInt("splice_region_in5");
		int splice_region_in_3 = getInt("splice_region_in3");

        if(max_dist_to_trans <= 0 || splice_region_ex <= 0 || splice_region_in_5 <= 0 || splice_region_in_3 <= 0)
        {
            THROW(CommandLineParsingException, "Distance to transcript and splice region parameters must be >= 1!");
        }

		QTime timer;
		QTextStream stream(stdout);

		//parse GFF file
		timer.start();
		GffData data;
		NGSHelper::loadGffFile(gff_file, data);
		stream << "Parsed " << QString::number(data.transcripts.count()) << " transcripts from input GFF file." << endl;
		stream << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;

		//remove transcripts not flagged as GENCODE basic
		if (!all)
		{
			timer.start();
			int c_removed = 0;
			for (int i=data.transcripts.count()-1; i>=0; --i)
			{
				QByteArray trans_id = data.transcripts[i].name();
				if (!data.gencode_basic.contains(trans_id))
				{
					data.transcripts.removeAt(i);
					++c_removed;
				}
			}
			stream << "Removed " << QString::number(c_removed) << " transcripts because they are not flagged as 'GENCODE basic'." << endl;
			stream << "Removing transcripts took: " << Helper::elapsedTime(timer) << endl;
		}

		//create transcript index
		data.transcripts.sortByPosition();
		ChromosomalIndex<TranscriptList> transcript_index(data.transcripts);

		//annotate
		timer.start();
		QPair<int, int> counts = annotateVcfStream(in_file, out_file, transcript_index, reference, max_dist_to_trans, splice_region_ex, splice_region_in_5, splice_region_in_3, tag);
		stream << "Annotated " << QString::number(counts.first) << " variants." << endl;
		stream << "Skipped " << QString::number(counts.second) << " variants with invalid ALT sequence." << endl;
		stream << "Annotating variants took: " << Helper::elapsedTime(timer) << endl;
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
