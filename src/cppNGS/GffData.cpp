#include "GffData.h"
#include "NGSHelper.h"
#include "VersatileFile.h"

int fastSplit(const QByteArray& line, QList<QByteArrayView>& parts, char sep)
{
    const char* data = line.constData();
    qsizetype start = 0;
    qsizetype col = 0;
    for (qsizetype i=0; i<line.size(); ++i)
    {
        if (data[i]==sep)
        {
            parts[col++] = QByteArrayView(data + start, i-start);
            start = i + 1;
        }
    }
    parts[col++] = QByteArrayView(data+start, line.size()-start);

    return col;
}

QList<QByteArrayView> fastSplit(const QByteArray& line, char sep)
{
    QList<QByteArrayView> output;

    const char* data = line.constData();
    qsizetype start = 0;
    for (qsizetype i=0; i<line.size(); ++i)
    {
        if (data[i]==sep)
        {
            output << QByteArrayView(data + start, i-start);
            start = i + 1;
        }
    }
    output << QByteArrayView(data+start, line.size()-start);

    return output;
}


QList<QByteArrayView> fastSplit(QByteArrayView line, char sep)
{
    QList<QByteArrayView> output;

    const char* data = line.data();
    qsizetype start = 0;
    for (qsizetype i = 0; i < line.size(); ++i)
    {
        if (data[i] == sep)
        {
            output << QByteArrayView(data + start, i - start);
            start = i + 1;
        }
    }
    output << QByteArrayView(data + start, line.size() - start);

    return output;
}

QHash<QByteArray, QByteArray> parseGffAttributes(const QByteArray& to_split)
{
    QHash<QByteArray, QByteArray> output;

    QList<QByteArrayView> parts = fastSplit(to_split, ';');
    foreach(const QByteArrayView& part, parts)
    {
        int split_index = part.indexOf('=');
        if (split_index==-1) continue;

		QByteArray key = part.first(split_index).trimmed().toByteArray();
		QByteArray value = part.sliced(split_index+1).trimmed().toByteArray();
        output[key] = value;
    }

    return output;
}

//Helper struct for GFF parsing of gene line:
//1	ensembl_havana	gene	65419	71585	.	+	.	ID=gene:ENSG00000186092;Name=OR4F5;biotype=protein_coding;description=olfactory receptor family 4 subfamily F member 5 [Source:HGNC Symbol%3BAcc:HGNC:14825];gene_id=ENSG00000186092;logic_name=ensembl_havana_gene_homo_sapiens;version=7
struct GeneData
{
    QByteArray gene_id;
    QByteArray gene_symbol;
    QByteArray hgnc_id;
};
GeneData parseGeneLine(QByteArrayView to_split)
{
    GeneData output;

    QList<QByteArrayView> parts = fastSplit(to_split, ';');
    foreach(const QByteArrayView& part, parts)
    {
        int split_index = part.indexOf('=');
        if (split_index==-1) continue;

		QByteArrayView key = part.first(split_index).trimmed();
		QByteArrayView value = part.sliced(split_index+1).trimmed();
        if (key=="Name")
        {
            output.gene_symbol = value.toByteArray();
        }
        else if (key=="gene_id")
        {
            output.gene_id = value.toByteArray();
        }
        else if (key=="description")
        {
            //extract HGNC identifier
            int start = value.indexOf("[Source:HGNC Symbol%3BAcc:");
            if (start!=-1)
            {
                start += 26;
                int end = value.indexOf("]", start);
                if (end!=-1)
                {
					output.hgnc_id = value.sliced(start, end-start).trimmed().toByteArray();
                }
            }
        }
    }

    return output;
}

//Helper struct for GFF parsing of transcript line:
//1	ensembl_havana	mRNA	685716	686654	.	-	.	ID=transcript:ENST00000332831;Parent=gene:ENSG00000284662;Name=OR4F16-201;biotype=protein_coding;ccdsid=CCDS41221.1;tag=gencode_basic,gencode_primary,Ensembl_canonical,MANE_Select;transcript_id=ENST00000332831;transcript_support_level=NA (assigned to previous version 3);version=5
struct TranscriptData
{
    QByteArray name;
    int version = 0;
    QByteArray name_ccds;
    QByteArray gene_symbol;
    QByteArray gene_id;
    QByteArray hgnc_id;
    Chromosome chr;
    int start_coding = 0;
    int end_coding = 0;
    Transcript::STRAND strand;
    Transcript::BIOTYPE biotype;
    bool is_gencode_basic = false;
    bool is_gencode_primary = false;
    bool is_ensembl_canonical = false;
    bool is_mane_select = false;
    bool is_mane_plus_clinical = false;

    BedFile exons;
};
TranscriptData parseTranscriptLine(QByteArrayView to_split)
{
    TranscriptData output;

    QList<QByteArrayView> parts = fastSplit(to_split, ';');
    foreach(const QByteArrayView& part, parts)
    {
        int split_index = part.indexOf('=');
        if (split_index==-1) continue;

		QByteArrayView key = part.first(split_index).trimmed();
		QByteArrayView value = part.sliced(split_index+1).trimmed();
        if (key=="Parent")
        {
            int sep = value.indexOf(':');
			output.gene_id = value.sliced(sep+1).toByteArray();
        }
        else if (key=="transcript_id")
        {
            output.name = value.toByteArray();
        }
        else if (key=="version")
        {
            output.version = Helper::toInt(value, "transcript version");
        }
        else if (key=="ccdsid")
        {
            output.name_ccds = value.toByteArray();
        }
        else if (key=="biotype")
        {
            output.biotype = Transcript::stringToBiotype(value.toByteArray());
        }
        else if (key=="tag")
        {
            QList<QByteArrayView> tags = fastSplit(value, ',');
            foreach(const QByteArrayView& tag, tags)
            {
                if (tag=="basic" || tag=="gencode_basic") //The tag was changed from "basic" in Ensembl 112 to "gencode_basic" in Ensembl 113
                {
                    output.is_gencode_basic = true;
                }
                else if (tag=="gencode_primary")
                {
                    output.is_gencode_primary = true;
                }
                else if (tag=="Ensembl_canonical")
                {
                    output.is_ensembl_canonical = true;
                }
                else if (tag=="MANE_Select")
                {
                    output.is_mane_select = true;
                }
                else if (tag=="MANE_Plus_Clinical")
                {
                    output.is_mane_plus_clinical = true;
                }
            }
        }
    }

    return output;
}

GffData GffData::load(QString filename, GffSettings settings)
{
    int c_skipped_special_chr = 0;
    QSet<QByteArray> special_chrs;
    int c_skipped_no_name_and_hgnc = 0;
    int c_skipped_low_evidence = 0;
    int c_skipped_not_hgnc = 0;
    //load data
    GffData data;
    if (settings.source=="ensembl") data = loadEnsembl(filename, settings, c_skipped_special_chr, special_chrs, c_skipped_no_name_and_hgnc, c_skipped_low_evidence, c_skipped_not_hgnc);
    else if (settings.source=="refseq") data = loadRefseq(filename, settings, c_skipped_special_chr, special_chrs, c_skipped_no_name_and_hgnc, c_skipped_low_evidence, c_skipped_not_hgnc);
    else THROW(ArgumentException, "Invalid GFF source '" + settings.source + "'!");

    //text output
    if (settings.print_to_stdout)
    {
        QTextStream out(stdout);
        out << "Parsed " << data.transcripts.geneCount() << " genes from GFF" << Qt::endl;
        out << "Parsed " << data.transcripts.count() << " transcripts from GFF" << Qt::endl;
        if (c_skipped_special_chr>0)
        {
            out << "Notice: " << QByteArray::number(c_skipped_special_chr) << " genes on special chromosomes skipped: " << special_chrs.values().join(", ") << Qt::endl;
        }
        if (c_skipped_no_name_and_hgnc>0)
        {
            out << "Notice: " << QByteArray::number(c_skipped_no_name_and_hgnc) << " genes without symbol and HGNC identifier skipped." << Qt::endl;
        }
        if (c_skipped_not_hgnc>0)
        {
            out << "Notice: " << QByteArray::number(c_skipped_not_hgnc) << " genes without a HGNC identifier skipped." << Qt::endl;
        }
        if (c_skipped_low_evidence>0)
        {

            out << "Notice: " << QByteArray::number(c_skipped_special_chr) << " transcipts not " << (settings.source=="ensembl" ? "flagged as 'GENCODE basic'" : "from data source RefSeq/BestRefSeq") << " skipped." << Qt::endl;
        }
    }

    return data;
}

GffData GffData::loadEnsembl(QString filename, const GffSettings& settings, int& c_skipped_special_chr, QSet<QByteArray>& special_chrs, int& c_skipped_no_name_and_hgnc, int& c_skipped_low_evidence, int& c_skipped_not_hgnc)
{
    GffData output;
    output.transcripts.reserve(225000); //224155 gencode basic transcripts in Ensembl 115...

    //init
    QHash<QByteArray, TranscriptData> transcripts;
    QHash<QByteArray, QByteArray> ensg2hgnc;

    QList<QByteArrayView> parts;
    parts.resize(9);

    VersatileFile stream(filename);
    stream.open(QFile::ReadOnly | QIODevice::Text);
    while(!stream.atEnd())
    {
        QByteArray line = stream.readLine(true);
        if (line.isEmpty()) continue;

        //section end => commit data
        if (line=="###")
        {
            //convert from TranscriptData to Transcript and append to list
            for(auto it = transcripts.begin(); it!=transcripts.end(); ++it)
            {
                TranscriptData& t_data = it.value();
                t_data.exons.merge();

                Transcript t;
                t.setGene(t_data.gene_symbol);
                t.setGeneId(t_data.gene_id);
                t.setHgncId(t_data.hgnc_id);
                t.setName(t_data.name);
                t.setVersion(t_data.version);
                t.setNameCcds(t_data.name_ccds);
                t.setSource(Transcript::ENSEMBL);
                t.setStrand(t_data.strand);
                t.setBiotype(t_data.biotype);
                int coding_start = t_data.start_coding;
                int coding_end = t_data.end_coding;
                if(t.strand() == Transcript::MINUS)
                {
                    int temp = coding_start;
                    coding_start = coding_end;
                    coding_end = temp;
                }
                t.setRegions(t_data.exons, coding_start, coding_end);
                t.setGencodeBasicTranscript(t_data.is_gencode_basic);
                t.setGencodePrimaryTranscript(t_data.is_gencode_primary);
                t.setEnsemblCanonicalTranscript(t_data.is_ensembl_canonical);
                t.setManeSelectTranscript(t_data.is_mane_select);
                t.setManePlusClinicalTranscript(t_data.is_mane_plus_clinical);

                output.transcripts << t;
            }

            //clear cache
            transcripts.clear();
            ensg2hgnc.clear();
            continue;
        }

        //skip header lines
        if (line.startsWith("#")) continue;

        int col_count = fastSplit(line, parts, '\t');
        if (col_count!=9) THROW(FileParseException, "GFF line of '" + filename + "' does not contain 9 columns:\n"+line);

        //gene line
        if (parts[8].startsWith("ID=gene:"))
        {
            GeneData line_data = parseGeneLine(parts[8]);

            // store mapping for pseudogene table
            output.ensg2symbol.insert(line_data.gene_id, line_data.gene_symbol);

            Chromosome chr(parts[0]);
            if (!chr.isNonSpecial())
            {
                special_chrs << chr.str();
                ++c_skipped_special_chr;
                continue;
            }

            if (line_data.gene_symbol.isEmpty() && line_data.hgnc_id.isEmpty())
            {
                ++c_skipped_no_name_and_hgnc;
                continue;
            }

            if (settings.skip_not_hgnc && line_data.hgnc_id.isEmpty())
            {
                ++c_skipped_not_hgnc;
                continue;
            }

            ensg2hgnc[line_data.gene_id] = line_data.hgnc_id;
        }

        //transcript line
        else if (parts[8].startsWith("ID=transcript:"))
        {
            TranscriptData tmp = parseTranscriptLine(parts[8]);

            //store mapping for pseudogene table
            output.enst2ensg.insert(tmp.name, tmp.gene_id);

            //store GENCODE basic data
            if (!settings.include_all && !tmp.is_gencode_basic)
            {
                ++c_skipped_low_evidence;
                continue;
            }

            //skip transcripts of skipped genes
            if(!ensg2hgnc.contains(tmp.gene_id)) continue;

            tmp.gene_symbol = output.ensg2symbol[tmp.gene_id];
            tmp.hgnc_id = ensg2hgnc[tmp.gene_id];
            tmp.chr = parts[0];;
            tmp.strand = Transcript::stringToStrand(parts[6].toByteArray());

            transcripts[tmp.name] = tmp;
        }

        //exon lines:
        //1	havana_tagene	exon	778739	779092	.	+	.	Parent=transcript:ENST00000457084;Name=ENSE00004083370;constitutive=0;ensembl_end_phase=-1;ensembl_phase=-1;exon_id=ENSE00004083370;rank=1;version=1
        else if (parts[2]=="CDS" || parts[2]=="exon" || parts[2]=="three_prime_UTR" || parts[2]=="five_prime_UTR" )
        {
            int enst_start = parts[8].indexOf("Parent=transcript:")+18;
            int enst_end = parts[8].indexOf(";", enst_start);
            if (enst_end==-1) enst_end=parts[8].size();
            QByteArray parent_id = parts[8].mid(enst_start, enst_end-enst_start).toByteArray();

            //skip exons of skipped transcripts
            if (!transcripts.contains(parent_id)) continue;

            TranscriptData& t_data = transcripts[parent_id];

            //check chromosome matches
            Chromosome chr(parts[0]);
            if (chr!=t_data.chr) THROW(FileParseException, "Chromosome mismatch between transcript and exon!");

            //update coding start/end
            int start = Helper::toInt(parts[3], "start position");
            int end = Helper::toInt(parts[4], "end position");

            if (parts[2]=="CDS")
            {
                t_data.start_coding = (t_data.start_coding==0) ? start : std::min(start, t_data.start_coding);
                t_data.end_coding = (t_data.end_coding==0) ? end : std::max(end, t_data.end_coding);
            }

            //add coding exon
            t_data.exons.append(BedLine(chr, start, end));
        }
    }
    return output;
}


GffData GffData::loadRefseq(QString filename, const GffSettings& settings, int& c_skipped_special_chr, QSet<QByteArray>& special_chrs, int& c_skipped_no_name_and_hgnc, int& c_skipped_low_evidence, int& c_skipped_not_hgnc)
{
    GffData output;

    //init
    QHash<QByteArray, Chromosome> id2chr; //refseq chromosome ID to normal chromosome name
    {
        QHash<Chromosome, QString> tmp = NGSHelper::chromosomeMapping(GenomeBuild::HG38);
        foreach (Chromosome key, tmp.keys())
        {
            id2chr.insert(tmp[key].toUtf8(), key);
        }
    }
    struct GeneInfo
    {
        QByteArray symbol;
        QByteArray hgnc;
        QByteArray biotype;
    };
    QHash<QByteArray, GeneInfo> geneid_to_data;
    QHash<QByteArray, TranscriptData> transcripts; //ID > data

    VersatileFile stream(filename);
    stream.open(QFile::ReadOnly | QIODevice::Text);
    while(!stream.atEnd())
    {
        QByteArray line = stream.readLine(true);
        if (line.isEmpty()) continue;

        //skip header lines
        if (line.startsWith("#")) continue;

        QByteArrayList parts = line.split('\t');

        QByteArray source = parts[1];
        if (!settings.include_all && !source.contains("RefSeq"))
        {
            ++c_skipped_low_evidence;
            continue;
        }

        const QByteArray& chr_string = parts[0];
        Chromosome chr = id2chr[chr_string];
        const QByteArray& details = parts[8];

        //gene line
        if (details.startsWith("ID=gene-"))
        {
            QHash<QByteArray, QByteArray> data = parseGffAttributes(details);


            if (!chr.isNonSpecial())
            {
                special_chrs << chr_string;
                ++c_skipped_special_chr;
                continue;
            }

            //extract HGNC identifier
            QByteArray hgnc_id;
            foreach(QByteArray entry, data["Dbxref"].split(','))
            {
                if (entry.startsWith("HGNC:"))
                {
                    hgnc_id = entry.mid(5);
                }
            }

            const QByteArray& gene = data["Name"];
            if (gene.isEmpty() && hgnc_id.isEmpty())
            {
                ++c_skipped_no_name_and_hgnc;
                continue;
            }

            if (settings.skip_not_hgnc && hgnc_id.isEmpty())
            {
                ++c_skipped_not_hgnc;
                continue;
            }

            const QByteArray& id = data["ID"];
            const QByteArray& gene_biotype = data["gene_biotype"];
            geneid_to_data[id] = GeneInfo{gene, hgnc_id, gene_biotype};
        }

        //transcript line
        else if (details.startsWith("ID=rna-"))
        {
            QHash<QByteArray, QByteArray> data = parseGffAttributes(details);

            //skip transcripts of skipped genes
            const QByteArray& gene_id = data["Parent"];
            if(!geneid_to_data.contains(gene_id)) continue;

            QByteArray name = data["Name"];
            int version = 0;
            int sep_idx = name.lastIndexOf('.');
            if (sep_idx!=-1)
            {
                version = Helper::toInt(name.mid(sep_idx+1), "transcript version", name);
                name = name.left(sep_idx);
            }
            TranscriptData tmp;
            tmp.name = name;
            tmp.version = version;
            tmp.name_ccds = "";
            tmp.gene_id = gene_id;
            const GeneInfo& gene_data = geneid_to_data[gene_id];
            tmp.gene_symbol = gene_data.symbol;
            tmp.hgnc_id = gene_data.hgnc;
            tmp.chr = chr;
            tmp.strand = Transcript::stringToStrand(parts[6]);
            tmp.biotype = Transcript::stringToBiotype(gene_data.biotype);
            tmp.is_gencode_basic = false;
            tmp.is_gencode_primary = false;
            tmp.is_ensembl_canonical = false;
            tmp.is_mane_select = false;
            tmp.is_mane_plus_clinical = false;
            transcripts[data["ID"]] = tmp;
        }

        //exon lines
        else
        {
            QByteArray type = parts[2];
            if (type=="CDS" || type=="exon" || type=="miRNA")
            {
                QHash<QByteArray, QByteArray> data = parseGffAttributes(details);

                //skip exons of skipped genes
                QByteArray transcript_id = data["Parent"];
                if (!transcripts.contains(transcript_id)) continue;

                TranscriptData& t_data = transcripts[transcript_id];

                //check chromosome matches
                if (chr!=t_data.chr)
                {
                    THROW(FileParseException, "Chromosome mismatch between transcript and exon!");
                }

                //update coding start/end
                int start = Helper::toInt(parts[3], "start position");
                int end = Helper::toInt(parts[4], "end position");

                if (type=="CDS")
                {
                    t_data.start_coding = (t_data.start_coding==0) ? start : std::min(start, t_data.start_coding);
                    t_data.end_coding = (t_data.end_coding==0) ? end : std::max(end, t_data.end_coding);
                }

                //add coding exon
                t_data.exons.append(BedLine(chr, start, end));
            }
        }
    }

    //convert from TranscriptData to Transcript and append to list
    output.transcripts.reserve(transcripts.count());
    for(auto it = transcripts.begin(); it!=transcripts.end(); ++it)
    {
        TranscriptData& t_data = it.value();
        t_data.exons.merge();

        Transcript t;
        t.setGene(t_data.gene_symbol);
        t.setGeneId(t_data.gene_id);
        t.setHgncId(t_data.hgnc_id);
        t.setName(t_data.name);
        t.setVersion(t_data.version);
        t.setNameCcds(t_data.name_ccds);
        t.setSource(Transcript::ENSEMBL);
        t.setStrand(t_data.strand);
        t.setBiotype(t_data.biotype);
        int coding_start = t_data.start_coding;
        int coding_end = t_data.end_coding;
        if(t.strand() == Transcript::MINUS)
        {
            int temp = coding_start;
            coding_start = coding_end;
            coding_end = temp;
        }
        t.setRegions(t_data.exons, coding_start, coding_end);
        t.setGencodeBasicTranscript(t_data.is_gencode_basic);
        t.setGencodePrimaryTranscript(t_data.is_gencode_primary);
        t.setEnsemblCanonicalTranscript(t_data.is_ensembl_canonical);
        t.setManeSelectTranscript(t_data.is_mane_select);
        t.setManePlusClinicalTranscript(t_data.is_mane_plus_clinical);

        output.transcripts << t;
    }

    return output;
}
