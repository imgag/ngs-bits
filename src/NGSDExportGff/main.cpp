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
        addFlag("test", "Uses the test database instead of on the production database.");
	}

	//TODO add test + refactoring to use Transcript class

    QList<QHash<QString, QByteArray>> get_genes() 
    {
        
        QList<QHash<QString, QByteArray>> genes;
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
            genes << newGene;

        }

        //qDebug() << genes; test
        return genes;
    }


    QHash<QByteArray, QList<QHash<QString, QByteArray>>> get_transcripts() 
    {
        QHash<QByteArray, QList<QHash<QString, QByteArray>>> transcripts;
        NGSD db(getFlag("test"));
        SqlQuery query = db.getQuery();
        query.exec("SELECT id, gene_id, name, chromosome, strand, biotype, is_gencode_basic, is_ensembl_canonical, is_mane_select, is_mane_plus_clinical, "
	               "     (SELECT MIN(start) start FROM gene_exon ge WHERE ge.transcript_id = gt.id group BY transcript_id) start,"
                   "     (SELECT MAX(end) start FROM gene_exon ge WHERE ge.transcript_id = gt.id group BY transcript_id) end"
                   " FROM gene_transcript gt "
                   " WHERE source = 'ensembl'");
		while (query.next())
		{
            QHash<QString, QByteArray> newTranscript{{"id", query.value("id").toByteArray()},
                                                     {"name", query.value("name").toByteArray()},
                                                     {"chromosome", query.value("chromosome").toByteArray()},
                                                     {"strand", query.value("strand").toByteArray()},
                                                     {"start", query.value("start").toByteArray()},
                                                     {"end", query.value("end").toByteArray()},
                                                     {"biotype", query.value("biotype").toByteArray()},
                                                     {"is_gencode_basic", query.value("is_gencode_basic").toByteArray()},
                                                     {"is_ensembl_canonical", query.value("is_ensembl_canonical").toByteArray()},
                                                     {"is_mane_select", query.value("is_mane_select").toByteArray()},
                                                     {"is_mane_plus_clinical", query.value("is_mane_plus_clinical").toByteArray()}
                                                };
            QByteArray currentGene = query.value("gene_id").toByteArray();
            if (transcripts.find(currentGene) == transcripts.end()) {
                QList<QHash<QString, QByteArray>> newList({newTranscript});
                transcripts[currentGene] = newList;
            } else {
                transcripts[currentGene] << newTranscript;
            }
            
        }

        //qDebug() << transcripts;
        return transcripts;
    }


    QHash<QByteArray, QList<QHash<QString, QByteArray>>> get_exons(){
        QHash<QByteArray, QList<QHash<QString, QByteArray>>>  exons;
        NGSD db(getFlag("test"));
        SqlQuery query = db.getQuery();
        query.exec("SELECT transcript_id, start, end FROM gene_exon WHERE transcript_id IN (SELECT id FROM gene_transcript WHERE source='ensembl')");
        //query.exec("SELECT transcript_id, start, end FROM gene_exon WHERE transcript_id IN (1031426, 1031425)");
		while (query.next())
		{
            QHash<QString, QByteArray> newExon{{"start", query.value("start").toByteArray()},
                                               {"end", query.value("end").toByteArray()}
                                            };
            QByteArray currentTranscript = query.value("transcript_id").toByteArray();
            if (exons.find(currentTranscript) == exons.end()) {
                QList<QHash<QString, QByteArray>> newList({newExon});
                exons[currentTranscript] = newList;
            } else {
                exons[currentTranscript] << newExon;
            }
        }

        //qDebug() << exons;
        return exons;
        
    }


	void write_gff_line(const QSharedPointer<QFile> outfile, const QByteArray chromosome, const QByteArray linetype, const QByteArray start, const QByteArray end, const QByteArray strand, const QByteArray info) {
			outfile->write(chromosome + "\t");
			outfile->write("NGSD\t");
			outfile->write(linetype + "\t");
			outfile->write(start + "\t");
			outfile->write(end + "\t");
			outfile->write(".\t");
			outfile->write(strand + "\t");
			outfile->write(".\t");
			outfile->write(info);
			outfile->write("\n");
	}


    virtual void main()
    {

        // open output file
        QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);

        
        // write header
        outfile->write("##gff-version 3\n");
		
		
		
        QList<QHash<QString, QByteArray>> genes = get_genes();
		
        QHash<QByteArray, QList<QHash<QString, QByteArray>>> transcripts = get_transcripts();
        
		QHash<QByteArray, QList<QHash<QString, QByteArray>>> exons = get_exons();


        // make build_one_tool_debug_noclean
        // /mnt/users/ahdoebm1/ngs-bits/bin/NGSDExportGff -out /mnt/users/ahdoebm1/test.gff3

        for (QHash<QString, QByteArray> gene : genes) {

            
            QList<QHash<QString, QByteArray>> currentTranscripts = transcripts[gene["id"]];

			if (currentTranscripts.length() == 0) {
				continue;
			}

			// write gene to file
			QByteArray strand = currentTranscripts[0]["strand"];
			QByteArray chrom = currentTranscripts[0]["chromosome"];
			QByteArray geneId = "gene:" + gene["id"];
			QByteArray info = "";
			info += "ID=" + geneId + ";";
			info += "Name=" + gene["symbol"] + ";";
			info += "Hgnc_id=" + gene["hgnc_id"] + ";";
			info += "Gene_id=" + gene["ensembl_id"] + ";";
			info += "Type=" + gene["type"] + ";";
			info += "Name=" + gene["name"];
			//2	ensembl_havana	gene	54972187	55112621	.	-	.	ID=gene:ENSG00000115310;Name=RTN4;biotype=protein_coding;description=reticulon 4 [Source:HGNC Symbol%3BAcc:HGNC:14085];gene_id=ENSG00000115310;logic_name=ensembl_havana_gene;version=17
			write_gff_line(outfile, chrom, "gene", gene["start"], gene["end"], strand, info);

            
			
            for(QHash<QString, QByteArray> transcript : currentTranscripts) {
				// write transcript to file
				QByteArray transcriptId = "transcript:" + transcript["id"]; 
				info = "";
				info += "ID=" + transcriptId + ";";
				info += "Name=" + gene["symbol"] + ";";
				info += "Parent=" + geneId + ";";
				info += "Transcript_id=" + transcript["name"] + ";";
				info += "Biotype=" + transcript["biotype"].replace(' ', '_') + ";";
				info += "Is_gencode_basic=" + transcript["is_gencode_basic"] + ";";
				info += "Is_ensembl_canonical=" + transcript["is_ensembl_canonical"] + ";";
				info += "Is_mane_select=" + transcript["is_mane_select"] + ";";
				info += "Is_mane_plus_clinical=" + transcript["is_mane_plus_clinical"];
				write_gff_line(outfile, transcript["chromosome"], "mRNA", transcript["start"], transcript["end"], transcript["strand"], info);

                QList<QHash<QString, QByteArray>> currentExons = exons[transcript["id"]];

				
        		//qDebug() << currentExons;

                
				for(QHash<QString, QByteArray> exon : currentExons) {
					// write exon to file
					info = "";
					info += "Parent=" + transcriptId;
					write_gff_line(outfile, transcript["chromosome"], "exon", exon["start"], exon["end"], transcript["strand"], info);
				}

            }
			
			

        }
    
    }

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
