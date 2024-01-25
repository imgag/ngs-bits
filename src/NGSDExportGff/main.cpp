#include "ToolBase.h"
#include <QDebug>
#include "NGSD.h"
#include "Helper.h"


class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
        setDescription("Writes all transcripts and exons of all genes to a gff3 file.");
		addOutfile("out", "The output GFF file.", false);
        addFlag("genes", "Add gene lines to group transcripts. This should be turned off when you want to use the file for IGV. This will also skip all transcripts which do not have a gene entry in NGSD.");
        addFlag("test", "Uses the test database instead of on the production database.");
	}

	void write_gff_line(const QSharedPointer<QFile> outfile, const QByteArray& chromosome, const QByteArray& linetype, const QByteArray& start, const QByteArray& end, const QByteArray& strand, const QByteArray& info, const QByteArray& phase) {
			outfile->write(chromosome + "\t");
			outfile->write("NGSD\t");
			outfile->write(linetype + "\t");
			outfile->write(start + "\t");
			outfile->write(end + "\t");
			outfile->write(".\t");
			outfile->write(strand + "\t");
			outfile->write(phase + "\t");
			outfile->write(info);
			outfile->write("\n");
	}


    virtual void main()
    {
        // open output file
        QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);

        // write header
        outfile->write("##gff-version 3\n");

        NGSD db(getFlag("test"));

        QList<QByteArray> parts;
        QByteArray info;
        QByteArray geneId;
        QByteArray st;
        QByteArray ed;

        const TranscriptList transcripts = db.transcripts();
        const QHash<QString, QHash<QString, QByteArray>> genes = get_genes();

        QByteArray last_gene_id = "-1";


        foreach(const Transcript& trans, transcripts)
        {
            const QByteArray gene_id = trans.geneId();
            const QByteArray chrom = trans.chr().strNormalized(true);
			const QByteArray strand = trans.strandToString(trans.strand());
			QByteArray phase = ".";
			//calculate codon offset
			if (strand == "+")
			{
				phase = QByteArray::number((trans.codingStart()-trans.start()) % 3);
			}
			else if (strand == "-")
			{
				phase = QByteArray::number(3 - ((trans.end()-trans.codingStart()) % 3));
			}

            if (getFlag("genes") && (gene_id == "")) {
                continue;
            }

            const QHash<QString, QByteArray> gene = genes[gene_id];

            if (getFlag("genes") && (gene_id != last_gene_id)) {
                // GENE LINE
                geneId = "gene:" + trans.geneId();
                parts.clear();
				parts.append("ID=" + geneId);
				parts.append("Name=" + gene["symbol"]);
                parts.append("hgnc_id=" + gene["hgnc_id"]);
                parts.append("gene_id=" + gene["ensembl_id"]);
                parts.append("type=" + gene["type"]);
                parts.append("description=" + gene["name"]);                
                info = parts.join(';');
				write_gff_line(outfile, chrom, "gene", gene["start"], gene["end"], strand, info, ".");
            }
            last_gene_id = gene_id;

            const QByteArray transcriptId = "transcript:" + trans.name(); 
            const QByteArray biotype = trans.biotypeToString(trans.biotype()).replace(' ', '_');

            // RNA LINE
            parts.clear();
			parts.append("ID=" + transcriptId);
			parts.append("Name=" + gene["symbol"]);
            if (getFlag("genes")) {
				parts.append("Parent=" + geneId);
            }
            parts.append("transcript_id=" + trans.name());
            parts.append("biotype=" + biotype);
            const QByteArray is_gencode_basic = trans.isGencodeBasicTranscript() ? "1" : "0";
            parts.append("is_gencode_basic=" + is_gencode_basic);
            const QByteArray is_ensembl_canonical = trans.isEnsemblCanonicalTranscript() ? "1" : "0";
            parts.append("is_ensembl_canonical=" + is_ensembl_canonical);
            const QByteArray is_mane_select = trans.isManeSelectTranscript() ? "1" : "0";
            parts.append("is_mane_select=" + is_mane_select);
            const QByteArray is_mane_plus_clinical = trans.isManePlusClinicalTranscript() ? "1" : "0";
            parts.append("is_mane_plus_clinical=" + is_mane_plus_clinical);
            info = parts.join(";");
            st.setNum(trans.start());
            ed.setNum(trans.end());

			write_gff_line(outfile, trans.chr().strNormalized(true), "RNA", st, ed, strand, info, ".");

            const BedFile& coding_regions = trans.codingRegions();

            if (!coding_regions.isEmpty()) {
                const BedFile& utr3prime = trans.utr3prime();
                for ( int i=0; i<utr3prime.count(); ++i )
                {
                    // 3prime UTR LINEs
                    const BedLine& reg = utr3prime[i];
                    st.setNum(reg.start());
                    ed.setNum(reg.end());
                    parts.clear();
					parts.append("Parent=" + transcriptId);
                    info = parts.join(";");
					write_gff_line(outfile, reg.chr().strNormalized(true), "three_prime_UTR", st, ed, strand, info, ".");
                }

				//TODO: optimize

				int cds_offset = 0;
				for ( int i=0; i<coding_regions.count(); ++i )
				{
					// CDS LINEs
					// for transcripts on the reverse strand: start with the last exon and run backwards
					const BedLine& coding_region = (trans.strand() == Transcript::MINUS)? coding_regions[(coding_regions.count()-1)-i]: coding_regions[i];
					st.setNum(coding_region.start());
					ed.setNum(coding_region.end());
					//save current offset
					phase = QByteArray::number((3-cds_offset)%3);
					//determine new offset:
					cds_offset = (cds_offset + coding_region.length()) % 3;
					parts.clear();
					parts.append("Parent=" + transcriptId);
					info = parts.join(";");
					write_gff_line(outfile, coding_region.chr().strNormalized(true), "CDS", st, ed, strand, info, phase);
				}

                const BedFile& utr5prime = trans.utr5prime();
                for ( int i=0; i<utr5prime.count(); ++i )
                {
                    // 5prime UTR LINEs
                    const BedLine& reg = utr5prime[i];
                    st.setNum(reg.start());
                    ed.setNum(reg.end());
                    parts.clear();
					parts.append("Parent=" + transcriptId);
                    info = parts.join(";");
					write_gff_line(outfile, reg.chr().strNormalized(true), "five_prime_UTR", st, ed, strand, info, ".");
                }
            } else {
                const BedFile& exons = trans.regions();
                for ( int i=0; i<exons.count(); ++i )
                {
                    // 5prime UTR LINEs
                    const BedLine& reg = exons[i];
                    st.setNum(reg.start());
                    ed.setNum(reg.end());
                    parts.clear();
					parts.append("Parent=" + transcriptId);
                    info = parts.join(";");
					write_gff_line(outfile, reg.chr().strNormalized(true), "exon", st, ed, strand, info, ".");
                }
            }
        }
    }

    
    QHash<QString, QHash<QString, QByteArray>> get_genes() 
    {
        QHash<QString, QHash<QString, QByteArray>> genes;
        NGSD db(getFlag("test"));
        SqlQuery query = db.getQuery();
		query.exec("SELECT id, symbol, hgnc_id, ensembl_id, type, name,"
	                    "(SELECT MIN(start) start FROM ( "
	                    "	SELECT gene_id, (SELECT MIN(start) start FROM gene_exon ge WHERE ge.transcript_id = gt.id group BY transcript_id) start "
	                    "FROM gene_transcript gt  "
	                    "WHERE source = 'ensembl')x_start "
	                    "WHERE x_start.gene_id = gene.id "
	                    "GROUP BY gene_id) start, "
 
	                    "(SELECT MAX(end) end FROM ( "
	                    "	SELECT gene_id, (SELECT MAX(end) start FROM gene_exon ge WHERE ge.transcript_id = gt.id group BY transcript_id) end "
	                    "FROM gene_transcript gt  "
	                    "WHERE source = 'ensembl')x_end "
	                    "WHERE x_end.gene_id = gene.id "
	                    "GROUP BY gene_id) end "
                    "FROM gene");
		while (query.next())
		{

            QHash<QString, QByteArray> newGene{{"id", query.value("id").toByteArray()},
                                            {"symbol", query.value("symbol").toByteArray()}, 
                                            {"hgnc_id", query.value("hgnc_id").toByteArray()}, 
                                            {"ensembl_id", query.value("ensembl_id").toByteArray()}, 
                                            {"type", query.value("type").toByteArray()}, 
                                            {"name", query.value("name").toByteArray()},
                                            {"start", query.value("start").toByteArray()},
                                            {"end", query.value("end").toByteArray()}
                                        };
            genes[query.value("ensembl_id").toByteArray()] = newGene;

        }

        //qDebug() << genes;
        return genes;
    }

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
