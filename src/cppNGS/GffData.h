#ifndef GFFDATA_H
#define GFFDATA_H

#include "cppNGS_global.h"
#include "Transcript.h"

//Settings for Gff loading
struct GffSettings
{
    QString source = "ensembl"; //source of the GFF file (Ensembl or RefSeq)
    bool include_all = false; //if set to false, skips transcripts that are not flagged as "GENCODE basic" (Ensembl) or are not from origin RefSeq/BestRefSeq (RefSeq)
    bool skip_not_hgnc = false; //skip transcripts without HGNC ID
    bool print_to_stdout = true; //print infos to stdout
};

//GFF file data
class CPPNGSSHARED_EXPORT GffData
{
public:
	//Transcripts
	TranscriptList transcripts;

	//Map from ENST to ENSG.
	QHash<QByteArray, QByteArray> enst2ensg;

	//Map from ENSG to gene symbol
	QHash<QByteArray, QByteArray> ensg2symbol;

    //Returns transcripts with features from a Ensembl GFF file, transcript_gene_relation (ENST>ENSG) and gene_name_relation (ENSG>gene symbol).
    static GffData load(QString filename, GffSettings settings);

protected:
    static GffData loadEnsembl(QString filename, const GffSettings& settings, int& c_skipped_special_chr, QSet<QByteArray>& special_chrs, int& c_skipped_no_name_and_hgnc, int& c_skipped_low_evidence, int& c_skipped_not_hgnc);
    static GffData loadRefseq(QString filename, const GffSettings& settings, int& c_skipped_special_chr, QSet<QByteArray>& special_chrs, int& c_skipped_no_name_and_hgnc, int& c_skipped_low_evidence, int& c_skipped_not_hgnc);
};

#endif // GFFDATA_H
