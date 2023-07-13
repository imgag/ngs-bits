#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Transcript.h"
#include <QDebug>
#include <QHash>
#include <QRegularExpressionMatchIterator>
#include <VcfFile.h>
#include "NGSHelper.h"
#include "BasicStatistics.h"


class ConcreteTool
        : public ToolBase
{
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    // original MaxEntScan code: http://hollywood.mit.edu/burgelab/maxent/download/fordownload/
    // VEP plugin MaxEntScan code: https://github.com/Ensembl/VEP_plugins/blob/release/109/MaxEntScan.pm#L642
    // The VEP plugin also contains the SWA
    // This tool is basically a C++ reimplementation of the VEP plugin


    virtual void setup()
    {
        setDescription("Annotates a VCF file with MaxEntScan scores.");
        //mandatory
        addInfile("gff", "Ensembl-style GFF file with transcripts, e.g. from https://ftp.ensembl.org/pub/release-107/gff3/homo_sapiens/Homo_sapiens.GRCh38.107.gff3.gz.", false);

        //optional
        addOutfile("out", "Output VCF file containing the MaxEntScan scores in the INFO column. Writes to stdout if unset", true);
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true);
        addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true);
        addFlag("all", "If set, all transcripts are imported (the default is to skip transcripts not labeled with the 'GENCODE basic' tag). Only used if -mes is given.");
        addFlag("mes", "If set executes the standard MaxEntScan algorithm.");
        addFlag("swa", "If set executes the sliding window approach of MaxEntScan.");
    }

    // public functions here
	void printTranscript(const Transcript& transcript)
	{
		QStringList output;
        output << transcript.name();
        output << transcript.chr().strNormalized(true);
		output << QString::number(transcript.start());
        output << QString::number(transcript.end());
        output << transcript.gene();
        qDebug() << output;
	}



    virtual void main()
    {

        //chr2	42656200	.	G	T	.	.	. maxent_ref=6.763;maxentscan_alt=4.688
        // ref sequence: TTTATTTATTTTTGGACAGGAGA
        // alt sequence: TTTATTTATTTTTGGACAGTAGA


        // read in input vcf
        QString in_file = getInfile("in");
        QSharedPointer<QFile> reader = Helper::openFileForReading(in_file, true);

        // open output file
        QString out_file = getOutfile("out");
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file, true);

        // read in reference genome
        QString ref_file = getInfile("ref");
        if (ref_file=="") ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
        FastaFileIndex reference(ref_file);

        // read in matrices
        score5_rest_ = read_matrix_5prime(":/resources/score5_matrix.tsv");
        score3_rest_ = read_matrix_3prime(":/resources/score3_matrix.tsv");

        // parse GFF file
		QTextStream stream(stdout);
		QTime timer;
		timer.start();

        QString gff_path = getInfile("gff");
        bool all = getFlag("all");
        GffSettings gff_settings;
		gff_settings.print_to_stdout = true;
		gff_settings.skip_not_gencode_basic = !all;
		GffData gff_file = NGSHelper::loadGffFile(gff_path, gff_settings);
        stream << "Parsed " << QString::number(gff_file.transcripts.count()) << " transcripts from input GFF file." << endl;
		stream << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;
        gff_file.transcripts.sortByPosition();

        //qDebug() << gff_file.transcripts[23547].name();
        //qDebug() << gff_file.transcripts[23547].regions()[0].start();

        // get splice site regions as bedfile
        ChromosomalIndex<TranscriptList> transcripts = ChromosomalIndex<TranscriptList>(gff_file.transcripts);


        bool do_mes = getFlag("mes"); 
        bool do_mes_swa = getFlag("swa");       

        bool newInfoHeadAdded = false;

        
        while(!reader->atEnd())
        {
            QByteArray line = reader->readLine();

            //skip empty lines
            if(line.trimmed().isEmpty()) continue;


            //write out headers unchanged
            if(line.startsWith("##"))
            {
                writer->write(line);
                if (do_mes && line.contains("ID=mes")){
                    qWarning() << "WARNING: found MaxEntScan info header. Will skip calculation of MaxEntScan scores";
                    do_mes = false;
                }
                if (do_mes_swa && line.contains("ID=mes_swa")){
                    qWarning() << "WARNING: found MaxEntScan sliding window approach info header. Will skip calculation of MaxEntScan SWA scores";
                    do_mes_swa = false;
                }
                continue;
            }

            if(!newInfoHeadAdded)
            {
                if (do_mes){
                    writer->write("##INFO=<ID=mes,Number=1,Type=String,Description=\"The MaxEntScan scores. FORMAT: A | separated list of maxentscan_ref&maxentscan_alt&transcript_name items.\">\n");
                }
                if (do_mes_swa) {
                    writer->write("##INFO=<ID=mes_swa,Number=1,Type=String,Description=\"The MaxEntScan SWA scores. FORMAT: A | separated list of maxentscan_ref_donor&maxentscan_alt_donor&maxentscan_donor_comp&maxentscan_ref_acceptor&maxentscan_alt_acceptor&maxentscan_acceptor_comp&transcript_name items.\">\n");
                }
                newInfoHeadAdded = true;
            }

            // write column header line
            if(line.startsWith("#"))
            {
                writer->write(line);
                continue;
            }


            line = line.trimmed();

            //split line and extract variant infos
            QList<QByteArray> parts = line.split('\t');
            if(parts.count() < 8) THROW(FileParseException, "VCF with too few columns: " + line);

            Chromosome chr = parts[0];
            Sequence ref = curate_sequence(parts[3]);
            Sequence alt = curate_sequence(parts[4]);
            int start = atoi(parts[1]);
            int end = start + ref.length() - 1;
            QList<QByteArray> info = parts[7].split(';');

            Variant variant = Variant(chr, start, end, ref, alt, info);




            //write out multi-allelic and structural variants without annotation
            // write out insertions and deletions without annotation
            if(alt.contains(',') || alt.startsWith("<") || !is_valid_sequence(variant.obs()) || !variant.isValid()) {
                writer->write(line);
                continue;
            }

            //qDebug() << variant.start();
            //qDebug() << variant.end();



            if (do_mes && (variant.start() == variant.end())) { // only calculate for small variants
                QList<QByteArray> all_mes_strings = runMES(variant, transcripts, reference);
                if (all_mes_strings.count() > 0) { // add to info column & remove . if it was there
                    if (variant.annotations()[0] == ".") {
                        variant.annotations().removeAt(0);
                    }
                    variant.annotations() << "mes=" + all_mes_strings.join('|');
                }
            }

            if (do_mes_swa) {
                QList<QByteArray> all_mes_swa_strings = runSWA(variant, transcripts, reference);
                if (all_mes_swa_strings.count() > 0) { // add to info column & remove . if it was there
                    if (variant.annotations()[0] == ".") {
                        variant.annotations().removeAt(0);
                    }
                    variant.annotations() << "mes_swa=" + all_mes_swa_strings.join('|');
                }
            }



        
            parts[7] = variant.annotations().join(';');
            writer->write(parts.join('\t') + '\n');


        }
    }

private:
    QHash<char,float> bgd_ = {
        {'A',0.27},
        {'C',0.23},
        {'G',0.23},
        {'T',0.27}
    };

    // constants for five prime
    QHash<char,float> cons15_ ={
        {'A',0.004},
        {'C',0.0032},
        {'G',0.9896},
        {'T',0.0032}
    };
    QHash<char,float> cons25_ ={
        {'A',0.0034},
        {'C',0.0039},
        {'G',0.0042},
        {'T',0.9884}
    };
    QHash<QByteArray,float> score5_rest_;

    // constants for three prime
    QHash<char,float> cons13_ ={
        {'A',0.9903},
        {'C',0.0032},
        {'G',0.0034},
        {'T',0.0030}
    };
    QHash<char,float> cons23_ ={
        {'A',0.0027},
        {'C',0.0037},
        {'G',0.9905},
        {'T',0.0030}
    };
    QHash<int,QHash<int,float>> score3_rest_;
    QHash<Sequence,float> maxent_cache;

    /*
    QByteArray collect_info(const QByteArray& old_info, const QByteArray& prefix, const QByteArray& new_info, const QByteArray& sep=";")
    {
        const QByteArray prefixed_new_info = prefix + new_info;
        if (old_info == "." || old_info.trimmed().isEmpty())
        {
            return prefixed_new_info;
        } else {
            return old_info + sep + prefixed_new_info;
        }
    }
    */

    Sequence curate_sequence(const Sequence& sequence) {
        return sequence.toUpper();
    }

    bool is_valid_sequence(const Sequence& sequence) {
        return QRegExp("[ACGT]*").exactMatch(sequence);
    }

    /*
    void write_vcf(const FastaFileIndex& reference) {
        QTime timer;
		timer.start();
        QString gff_path = "/mnt/storage2/users/ahdoebm1/MaxEntScanStats/input/MANE_transcripts.gff.gz";
        GffSettings gff_settings;
		gff_settings.print_to_stdout = true;
		gff_settings.skip_not_gencode_basic = false;
		GffData gff_file = NGSHelper::loadGffFile(gff_path, gff_settings);
        QTextStream stream(stdout);
        stream << "Parsed " << QString::number(gff_file.transcripts.count()) << " transcripts from input GFF file." << endl;
		stream << "Parsing transcripts took: " << Helper::elapsedTime(timer) << endl;
        gff_file.transcripts.sortByPosition();

        QSharedPointer<QFile> writer = Helper::openFileForWriting("/mnt/storage2/users/ahdoebm1/MaxEntScanStats/output/mane_variants.vcf", true);


        foreach(Transcript transcript, gff_file.transcripts) {
            BedFile coding_regions = transcript.codingRegions();
            for ( int i=0; i<coding_regions.count(); ++i ) {
            
                BedLine coding_region = coding_regions[i];

                if (i != 0) {
                    int position = coding_region.start() - 1;
                    Sequence ref = reference.seq(coding_region.chr(), position, 1);
                    Sequence alt = ref.replace("G", "T");
                    //#CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO
                    QList<QByteArray> vcf_line({coding_region.chr().strNormalized(true), QByteArray::number(position), ".", ref, alt, ".", "transcript=" + transcript.name()});
                    writer->write(vcf_line.join('\t') + '\n');
                }
                if (i != coding_regions.count()) {
                    int position = coding_region.end() + 1;
                    Sequence ref = reference.seq(coding_region.chr(), position, 1);
                    Sequence alt = ref.replace("G", "T");
                    //#CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO
                    QList<QByteArray> vcf_line({coding_region.chr().strNormalized(true), QByteArray::number(position), ".", ref, alt, ".", "transcript=" + transcript.name()});
                    writer->write(vcf_line.join('\t') + '\n');
                }
            
            }
        }


    }
    */


    QList<QByteArray> runSWA(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts, const FastaFileIndex& reference) {
        QList<QByteArray> all_mes_swa_strings;

        QVector<int> transcripts_oi = transcripts.matchingIndices(variant.chr(), variant.start(), variant.end());

        foreach(int transcript_index, transcripts_oi) {
            Transcript current_transcript = transcripts.container()[transcript_index];
            Sequence ref_context;
            Sequence alt_context;

            // 5 prime ss / donor ss
            //Sequence ref_context = reference.seq(variant.chr(), variant.start()-8, 17);
            QList<Sequence> donor_seqs = get_seqs(variant, variant.start()-8, variant.end()+8, 17, reference, current_transcript);
            //qDebug() << donor_seqs;
            ref_context = donor_seqs[0];
            alt_context = donor_seqs[1];
            QList<float> max_ref_donor = get_max_score(ref_context, 9, &ConcreteTool::score5);
            QList<float> max_alt_donor = get_max_score(alt_context, 9, &ConcreteTool::score5);

            float donor_comp;
            if (variant.ref().length() == variant.obs().length()) {
                int donor_alt_frame = static_cast<int>(max_alt_donor[1]);
                Sequence donor_comp_seq = ref_context.mid(donor_alt_frame, 9);
                donor_comp = score_maxent(donor_comp_seq, &ConcreteTool::score5);
            } else {  // take the max ref score
                donor_comp = max_ref_donor[0];
            }
            


            // 3 prime ss / acceptor ss
            QList<Sequence> acceptor_seqs = get_seqs(variant, variant.start()-22, variant.end()+22, 45, reference, current_transcript);
            //qDebug() << acceptor_seqs;
            ref_context = acceptor_seqs[0];
            alt_context = acceptor_seqs[1];
            QList<float> max_ref_acceptor = get_max_score(ref_context, 23, &ConcreteTool::score3);
            QList<float> max_alt_acceptor = get_max_score(alt_context, 23, &ConcreteTool::score3);

            float acceptor_comp;
            if (variant.start() == variant.end()) {
                int acceptor_alt_frame = static_cast<int>(max_alt_acceptor[1]);
                Sequence acceptor_comp_seq = ref_context.mid(acceptor_alt_frame, 23);
                acceptor_comp = score_maxent(acceptor_comp_seq, &ConcreteTool::score3);
            } else {  // take the max ref score
                acceptor_comp = max_ref_acceptor[0];
            }

            // save 
            QList<QByteArray> new_mes({QByteArray::number(max_ref_donor[0]), QByteArray::number(max_alt_donor[0]), QByteArray::number(donor_comp), QByteArray::number(max_ref_acceptor[0]), QByteArray::number(max_alt_acceptor[0]), QByteArray::number(acceptor_comp), current_transcript.name()});
            QByteArray new_mes_string = new_mes.join('&');
            all_mes_swa_strings.append(new_mes_string);
        }

        return all_mes_swa_strings;
    }

    QList<float> get_max_score(const Sequence& context, const float& window_size, float (ConcreteTool::*scorefunc)(const Sequence&)) {
        float maxscore = -1 * std::numeric_limits<int>::max();
        int frame = -1;
        for (int i = 1; i <= context.length() - window_size + 1; i++) {
            Sequence current_sequence = context.mid(i, window_size);
            float current_score = score_maxent(current_sequence, scorefunc); //(this->*scorefunc)(current_sequence);
            if (current_score > maxscore) {
                maxscore = current_score;
                frame = i;
            }
        }
        QList<float> result({maxscore, static_cast<float>(frame)});
        return result;
    }










    QList<QByteArray> runMES(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts, const FastaFileIndex& reference) {
        QList<QByteArray> all_mes_strings;
        QVector<int> transcripts_oi = transcripts.matchingIndices(variant.chr(), variant.start(), variant.end());

        //qDebug() << transcripts_oi;

        // get splice site regions of transcripts oi
        foreach(int transcript_index, transcripts_oi) {
            Transcript current_transcript = transcripts.container()[transcript_index];
            //printTranscript(current_transcript);


            BedFile coding_regions = current_transcript.codingRegions();
            for ( int i=0; i<coding_regions.count(); ++i ) {
                BedLine coding_region = coding_regions[i];
                bool overlaps_three_prime = false;
                bool overlaps_five_prime = false;
                int slice_start_three;
                int slice_end_three;
                int slice_start_five;
                int slice_end_five;




                // EEEEEEEEEEEEGUXXXIIIIIIIIIIIIIIAPPPXXAGEEEEEEEEEEEE
                // -----Exon---5'ss-----Intron------3'ss------Exon----

                if (current_transcript.isPlusStrand()) {
                    // check 5 prime ss if it is not the first exon (bc. there is no 5' ss)
                    if (i != 0) {
                        slice_start_three = coding_region.start() - 20;
                        slice_end_three = coding_region.start() + 2;
                        overlaps_three_prime = variant.overlapsWith(slice_start_three, slice_end_three);
                    }
                    if (i != coding_regions.count()) {
                        slice_start_five = coding_region.end() - 2;
                        slice_end_five = coding_region.end() + 6;
                        overlaps_five_prime = variant.overlapsWith(slice_start_five, slice_end_five);
                    }
                } else {
                    if (i != 0) {
                        slice_start_three = coding_region.end() - 2;
                        slice_end_three = coding_region.end() + 20;
                        overlaps_three_prime = variant.overlapsWith(slice_start_three, slice_end_three);
                    }
                    if (i != coding_regions.count()) {
                        slice_start_five = coding_region.start() - 6;
                        slice_end_five = coding_region.start() + 2;
                        overlaps_five_prime = variant.overlapsWith(slice_start_five, slice_end_five);
                    }
                }


                if (overlaps_three_prime) {
                    // get sequences
                    QList<Sequence> seqs = get_seqs(variant, slice_start_three, slice_end_three, 23, reference, current_transcript);
                    Sequence ref_seq = seqs[0];
                    Sequence alt_seq = seqs[1];

                    // get scores
                    float maxentscan_ref = score_maxent(ref_seq, &ConcreteTool::score3);
                    float maxentscan_alt = score_maxent(alt_seq, &ConcreteTool::score3);

                    /*
                    qDebug() << ref_seq;
                    qDebug() << alt_seq;
                    qDebug() << "refscore three:" << maxentscan_ref;
                    qDebug() << "altscore three:" << maxentscan_alt;
                    */

                    // save 
                    QList<QByteArray> new_mes({QByteArray::number(maxentscan_ref), QByteArray::number(maxentscan_alt), current_transcript.name()});
                    QByteArray new_mes_string = new_mes.join('&');
                    all_mes_strings.append(new_mes_string);

                }

                if (overlaps_five_prime) {
                    // get sequences
                    QList<Sequence> seqs = get_seqs(variant, slice_start_five, slice_end_five, 9, reference, current_transcript);
                    Sequence ref_seq = seqs[0];
                    Sequence alt_seq = seqs[1];

                    // get scores
                    float maxentscan_ref = score_maxent(ref_seq, &ConcreteTool::score5);
                    float maxentscan_alt = score_maxent(alt_seq, &ConcreteTool::score5);

                    /*
                    qDebug() << ref_seq;
                    qDebug() << alt_seq;
                    qDebug() << "refscore three:" << maxentscan_ref;
                    qDebug() << "altscore three:" << maxentscan_alt;
                    */

                    // save 
                    QList<QByteArray> new_mes({QByteArray::number(maxentscan_ref), QByteArray::number(maxentscan_alt), current_transcript.name()});
                    QByteArray new_mes_string = new_mes.join('&');
                    all_mes_strings.append(new_mes_string);
                }


                    
            }


        }

        return all_mes_strings;
    }


    QList<Sequence> get_seqs(const Variant& variant, const int& slice_start, const int& slice_end, const int& length, const FastaFileIndex& reference, const Transcript& transcript) {
        Sequence ref_seq = reference.seq(variant.chr(), slice_start, length + variant.end() - variant.start());
        if (!is_valid_sequence(ref_seq)) {
            THROW(Exception, "Invalid reference sequence encountered in variant: " + variant.toString())
        }
        Sequence alt_base = variant.obs();
        int variant_position_in_slice = variant.start() - slice_start;
        if (transcript.isMinusStrand()) {
            ref_seq.reverseComplement();
            alt_base.reverseComplement();
            variant_position_in_slice = slice_end - slice_start - variant_position_in_slice;
        }
        Sequence alt_seq = ref_seq;
        alt_seq = alt_seq.replace(variant_position_in_slice, variant.ref().length(), alt_base);
        if (!is_valid_sequence(alt_seq)) {
            THROW(Exception, "Invalid alternative sequence encountered in variant: " + variant.toString())
        }

        QList<Sequence> result({ref_seq, alt_seq});
        return result;
    }


    
    float score_maxent(const Sequence& sequence, float (ConcreteTool::*scorefunc)(const Sequence&)) {
        float maxent_score;
        if (maxent_cache.contains(sequence)) { // check cache for reference sequence
            maxent_score = maxent_cache[sequence];
        } else { // calculate new in case not found & save to cache
            maxent_score = (this->*scorefunc)(sequence);
            maxent_cache[sequence] = maxent_score;
        }
        return maxent_score;
    }


    float score5(const Sequence& sequence) {
        float consensus_score = score5_consensus(sequence);
        float rest_score = score5_rest(sequence);
        return log2(consensus_score * rest_score);
    }


    float score5_consensus(const Sequence& sequence) {
        char seq_pos_3 = sequence[3];
        char seq_pos_4 = sequence[4];
        return cons15_.value(seq_pos_3)*cons25_.value(seq_pos_4)/(bgd_.value(seq_pos_3)*bgd_.value(seq_pos_4));
    }

    float score5_rest(const Sequence& sequence) {
        QByteArray rest_seq = sequence.mid(0,3) + sequence.mid(5);
        return score5_rest_[rest_seq];
    }


    float score3(const Sequence& sequence) {
        float consensus_score = score3_consensus(sequence);
        float rest_score = score3_rest(sequence);
        return log2(consensus_score * rest_score);
    }


    float score3_consensus(const Sequence& sequence) {
        char seq_pos_18 = sequence[18];
        char seq_pos_19 = sequence[19];
        return cons13_.value(seq_pos_18)*cons23_.value(seq_pos_19)/(bgd_.value(seq_pos_18)*bgd_.value(seq_pos_19));
    }

    float score3_rest(const Sequence& sequence) {
        QByteArray rest_seq = sequence.mid(0,18) + sequence.mid(20);
        float rest_score = 1;
        rest_score *= score3_rest_[0][hashseq(rest_seq.mid(0,7))];
        rest_score *= score3_rest_[1][hashseq(rest_seq.mid(7,7))];
        rest_score *= score3_rest_[2][hashseq(rest_seq.mid(14,7))];
        rest_score *= score3_rest_[3][hashseq(rest_seq.mid(4,7))];
        rest_score *= score3_rest_[4][hashseq(rest_seq.mid(11,7))];
        rest_score /= score3_rest_[5][hashseq(rest_seq.mid(4,3))];
        rest_score /= score3_rest_[6][hashseq(rest_seq.mid(7,4))];
        rest_score /= score3_rest_[7][hashseq(rest_seq.mid(11,3))];
        rest_score /= score3_rest_[8][hashseq(rest_seq.mid(14,4))];
        return rest_score;
    }

    int base_to_int(const char base) {
        if (base == 65) { // A
            return 0;
        }
        else if (base == 67){ // C
            return 1;
        }
        else if (base == 71){ // G
            return 2;
        }
        else if (base == 84){ // T
            return 3;
        } else {
            THROW (Exception, "Unknown base encountered: " + base)
        }
    }


    int hashseq(const QByteArray& sequence) {
        //QByteArray sequence_numbers = sequence;
        //sequence_numbers = sequence_numbers.replace('A', '0');
        //sequence_numbers = sequence_numbers.replace('C', '1');
        //sequence_numbers = sequence_numbers.replace('G', '2');
        //sequence_numbers = sequence_numbers.replace('T', '3');
        QList<int> pow_four({1,4,16,64,256,1024,4096,16384});
        int result = 0;
        int seqlength = sequence.length();
        for (int i = 0; i<seqlength; i++) {
            result += base_to_int(sequence[i]) * pow_four[seqlength - i - 1];
            //result += ((int) sequence_numbers[i]) * pow(4, sequence_numbers.length() - i - 1);
        }
        return result;
    }


    QHash<QByteArray,float> read_matrix_5prime(const QByteArray& path) {
        QHash<QByteArray,float> result;
        QStringList lines = Helper::loadTextFile(path, true, '#', true);
        foreach(const QString& line, lines){
            QByteArrayList parts = line.toUtf8().split('\t');
            if (parts.count()==2){
                QByteArray sequence = parts[0];
                float score = parts[1].toFloat();
                result.insert(sequence, score);
            }
        }
        return result;
    }

    QHash<int,QHash<int,float>> read_matrix_3prime(const QByteArray& path) {
        QHash<int,QHash<int,float>> result;
        QHash<int,float> current_inner;
        QStringList lines = Helper::loadTextFile(path, true, '#', true);
        int last_index = -1;

        foreach(const QString& line, lines){
            QByteArrayList parts = line.toUtf8().split('\t');
            
            if (parts.count()==3){
                int index = parts[0].toInt();
                int sequence_hash = parts[1].toInt();
                float score = parts[2].toFloat();
                if (last_index != index) {
                    result.insert(last_index, current_inner);
                    current_inner = {{sequence_hash, score}};
                } else {
                    current_inner.insert(sequence_hash, score);
                }

                last_index = index;
            }
        }
        result.insert(last_index, current_inner);
        return result;
    }

};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
