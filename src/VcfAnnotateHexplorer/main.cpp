#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Transcript.h"
#include <QDebug>
#include <QHash>
#include <QRegularExpressionMatchIterator>
#include "Settings.h"

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

    // online calculator for HEXPLORER & HBOND scores: https://www2.hhu.de/rna/html/hexplorer_score.php
    // paper: https://academic.oup.com/nar/article/42/16/10681/2903056?login=true

	virtual void setup()
	{
        setDescription("Annotates a VCF with Hexplorer and HBond scores.");
        addOutfile("out", "Output VCF file containing HEXplorer and HBOND scores in the INFO column.", false);
		//optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true);
        addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true);

	}

    float calculateHZEIforSequence(const QString& sequence)
    {
        float hzei_sum = 0;

        // iterate over all undecamers
        for (int i = 5; i < sequence.length()-5; i++)
        {
            QString current_11nt = sequence.mid(i-5, 11);
            float current_hzei = calculateHZEIperNT(current_11nt);
            //qDebug() << current_11nt;
            //qDebug() << current_hzei;
            hzei_sum += current_hzei;
        }

        // normalize the total HZEI score by the number of nucleotides where the HZEI score was calculated
        float result = hzei_sum / (sequence.length()-10);

        return result;
    }


    // this function calculates the hexplorer score for one nucleotide
    // thus, the sequence which is input to this function should only be 11 bases long
    float calculateHZEIperNT(const QString& sequence)
    {
        // sequence is assumed to be valid at this point!

        float zscore_sum = 0;

        for (int i = 0; i < 6; i++)
        {
            QString current_hexamer = sequence.mid(i, 6);
            zscore_sum += hexplorer_zscores_.value(current_hexamer, 0);
            //qDebug() << current_hexamer;
            //qDebug() << hexplorer_zscores_.value(current_hexamer, 0);
        }

        float result = zscore_sum/6; // take the mean of all 6 hexamer z scores

        return result;
    }


    float getMaxHBondScores(const QString& sequence)
    {
        QRegularExpression rx(".{3}GT.{6}");

        float result = 0;

        QRegularExpressionMatchIterator i = rx.globalMatch(sequence);
        while (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch())
            {
                 QString curent_sequence = match.captured(0);
                 float current_hbond_score = hbond_scores_.value(curent_sequence, 0);
                 result = std::max(current_hbond_score, result);
            }
        }
        return result;
    }


    virtual void main()
    {
        // read in input vcf
        QString in_file = getInfile("in");
        QSharedPointer<QFile> reader = Helper::openFileForReading(in_file, true);

        // open output file
        QString out_file = getOutfile("out");
        QSharedPointer<QFile> writer = Helper::openFileForWriting(out_file, false);

        // read in reference genome
        QString ref_file = getInfile("ref");
        if (ref_file=="") ref_file = Settings::string("reference_genome", true);
        if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
        //FastaFileIndex ref_index(ref_file);
        FastaFileIndex reference(ref_file);


        // parse hexplorer z scores table
        QStringList lines = Helper::loadTextFile(":/resources/HEXplorer_Z_scores.csv", true, '#', true);
        foreach(const QString& line, lines)
        {
            QByteArrayList parts = line.toUtf8().split(';');
            if (parts.count()==2)
            {
                QString hexamer = parts[0];
                float zscore = parts[1].toFloat();
                hexplorer_zscores_.insert(hexamer, zscore);
            }
        }

        // parse H-Bond scores table
        lines = Helper::loadTextFile(":/resources/H_Bond_score_table.csv", true, '#', true);
        foreach(const QString& line, lines)
        {
            QByteArrayList parts = line.toUtf8().split(';');
            if (parts.count()==2)
            {
                QString undecamer = parts[0];
                float score = parts[1].toFloat();
                hbond_scores_.insert(undecamer, score);
            }
        }


        bool new_info_head_added = false;
        bool add_hexplorer = true;
        bool add_hbond_score = true;

        while(!reader->atEnd())
        {
            QByteArray line = reader->readLine();

            //skip empty lines
            if(line.trimmed().isEmpty()) continue;


            //write out headers unchanged
            if(line.startsWith("##"))
            {
                writer->write(line);
                if (add_hexplorer && line.contains("hexplorer"))
                {
                    qWarning() << "WARNING: found hexplorer info header. Will skip calculation of hexplorer scores";
                    add_hexplorer = false;
                }
                if (add_hbond_score && line.contains("max_hbond"))
                {
                    qWarning() << "WARNING: found hbond score info header. Will skip calculation of hbond scores";
                    add_hbond_score = false;
                }
                continue;
            }

            //add the new info headers after all other header lines
            if(!new_info_head_added)
            {
                if (add_hexplorer)
                {
                    writer->write("##INFO=<ID=hexplorer_delta,Number=1,Type=Float,Description=\"This is the HEXplorer delta score (HZEI mutant - HZEI wildtype). HZEI scores were normalized by the total number of nucleotide positions which contribute to the score.\">\n");
                    writer->write("##INFO=<ID=hexplorer_mut,Number=1,Type=Float,Description=\"This is the HEXplorer score for the mutant sequence. HZEI scores were normalized by the total number of nucleotide positions which contribute to the score.\">\n");
                    writer->write("##INFO=<ID=hexplorer_wt,Number=1,Type=Float,Description=\"This is the HEXplorer score for the reference sequence. HZEI scores were normalized by the total number of nucleotide positions which contribute to the score.\">\n");
                    writer->write("##INFO=<ID=hexplorer_delta_rev,Number=1,Type=Float,Description=\"This is the HEXplorer delta score for the reverse complement of the original sequence (HZEI mutant rev - HZEI wildtype rev). HZEI scores were normalized by the total number of nucleotide positions which contribute to the score.\">\n");
                    writer->write("##INFO=<ID=hexplorer_mut_rev,Number=1,Type=Float,Description=\"This is the HEXplorer score for the reverse complement of the mutant sequence. HZEI scores were normalized by the total number of nucleotide positions which contribute to the score.\">\n");
                    writer->write("##INFO=<ID=hexplorer_wt_rev,Number=1,Type=Float,Description=\"This is the HEXplorer score for the reverse complement of the reference sequence. HZEI scores were normalized by the total number of nucleotide positions which contribute to the score.\">\n");
                }

                if (add_hbond_score)
                {
                    writer->write("##INFO=<ID=max_hbond_delta,Number=1,Type=Float,Description=\"This is the HBond delta score (max HBond mutant - max HBond wildtype).\">\n");
                    writer->write("##INFO=<ID=max_hbond_mut,Number=1,Type=Float,Description=\"This is the max HBond score for the mutant sequence.\">\n");
                    writer->write("##INFO=<ID=max_hbond_wt,Number=1,Type=Float,Description=\"This is the max HBond score for the reference sequence.\">\n");
                    writer->write("##INFO=<ID=max_hbond_delta_rev,Number=1,Type=Float,Description=\"This is the max HBond delta score for the reverse complement of the original sequence (HZEI mutant rev - HZEI wildtype rev).\">\n");
                    writer->write("##INFO=<ID=max_hbond_mut_rev,Number=1,Type=Float,Description=\"This is the max HBond score for the reverse complement of the mutant sequence.\">\n");
                    writer->write("##INFO=<ID=max_hbond_wt_rev,Number=1,Type=Float,Description=\"This is the max HBond score for the reverse complement of the reference sequence.\">\n");
                }

                new_info_head_added = true;
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
            int start = atoi(parts[1]);
            Sequence ref = parts[3].toUpper();
            Sequence alt = parts[4].toUpper();
            QByteArray info = parts[7];

            //write out multi-allelic and structural variants without annotation
            if(alt.contains(',') || alt.startsWith("<"))
            {
                writer->write(line);
                continue;
            }

            //qDebug() << chr.str() << ":" << start << " " << ref << ">" << alt;

            int wt_end = start + ref.length();
            int wt_seq_length = wt_end - start + 20;
            Sequence wt_seq = reference.seq(chr, start-10, wt_seq_length);
            //qDebug() << wt_seq;

            int mut_end = start + alt.length();
            Sequence mut_seq_prefix = reference.seq(chr, start-10, 10);
            Sequence mut_seq_postfix = reference.seq(chr, mut_end, 10);
            Sequence mut_seq = mut_seq_prefix + alt + mut_seq_postfix;
            //qDebug() << mut_seq;


            if (!(isValidSequence(wt_seq) && isValidSequence(mut_seq)))
            {
                qWarning() << "Skipping variant because it contains non ACGT letters: " << chr.str() << ":" << start << " " << ref << ">" << alt << " extended sequences: wt:" << wt_seq << ", mutant: " << mut_seq;
                writer->write(parts.join('\t')+'\n');
                continue;
            }


            if (add_hexplorer) {
                float hzei_wt = calculateHZEIforSequence(wt_seq);
                float hzei_mut = calculateHZEIforSequence(mut_seq);

                wt_seq.reverseComplement();
                mut_seq.reverseComplement();

                float hzei_wt_rev = calculateHZEIforSequence(wt_seq);
                float hzei_mut_rev = calculateHZEIforSequence(mut_seq);

                // recover original sequence in case we want to calculate hbond scores
                wt_seq.reverseComplement();
                mut_seq.reverseComplement();

                float delta_hzei = hzei_mut - hzei_wt;
                float delta_hzei_rev = hzei_mut_rev - hzei_wt_rev;

				info = collect_info(info, "hexplorer_delta=", QByteArray::number(delta_hzei, 'f', 2));
				info = collect_info(info, "hexplorer_mut=", QByteArray::number(hzei_mut, 'f', 2));
				info = collect_info(info, "hexplorer_wt=", QByteArray::number(hzei_wt, 'f', 2));
				info = collect_info(info, "hexplorer_delta_rev=", QByteArray::number(delta_hzei_rev, 'f', 2));
				info = collect_info(info, "hexplorer_mut_rev=", QByteArray::number(hzei_mut_rev, 'f', 2));
				info = collect_info(info, "hexplorer_wt_rev=", QByteArray::number(hzei_wt_rev, 'f', 2));
            }

            if (add_hbond_score) {
                float maxHbondScoreWT = getMaxHBondScores(wt_seq);
                float maxHbondScoreMUT = getMaxHBondScores(mut_seq);

                wt_seq.reverseComplement();
                mut_seq.reverseComplement();

                float maxHbondScoreWTRev = getMaxHBondScores(wt_seq);
                float maxHbondScoreMUTRev = getMaxHBondScores(mut_seq);

				float deltaHbondScore = maxHbondScoreMUT - maxHbondScoreWT;
                float deltaHbondScoreRev = maxHbondScoreMUTRev - maxHbondScoreWTRev;

                if (maxHbondScoreMUT > 0 || maxHbondScoreWT > 0) {
					info = collect_info(info, "max_hbond_delta=", QByteArray::number(deltaHbondScore, 'f', 2));
                }
                if (maxHbondScoreMUT > 0) {
					info = collect_info(info, "max_hbond_mut=", QByteArray::number(maxHbondScoreMUT, 'f', 2));
                }
                if (maxHbondScoreWT > 0) {
					info = collect_info(info, "max_hbond_wt=", QByteArray::number(maxHbondScoreWT, 'f', 2));
                }
                if (maxHbondScoreMUTRev > 0 || maxHbondScoreWTRev > 0) {
					info = collect_info(info, "max_hbond_delta_rev=", QByteArray::number(deltaHbondScoreRev, 'f', 2));
                }
                if (maxHbondScoreMUTRev > 0) {
					info = collect_info(info, "max_hbond_mut_rev=", QByteArray::number(maxHbondScoreMUTRev, 'f', 2));
                }
                if (maxHbondScoreWTRev > 0) {
					info = collect_info(info, "max_hbond_wt_rev=", QByteArray::number(maxHbondScoreWTRev, 'f', 2));
                }
            }

            parts[7] = info;
            writer->write(parts.join('\t')+'\n');

        }
    }

private:
    QHash<QString, float> hexplorer_zscores_;
    QHash<QString, float> hbond_scores_;


    QString curateSequence(QString sequence)
    {
        sequence = sequence.toUpper();
        return sequence;
    }


    bool isValidSequence(const QString& sequence)
    {
        return QRegularExpression(QRegularExpression::anchoredPattern("[ACGTacgt]*")).match(sequence).hasMatch();
    }

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
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
