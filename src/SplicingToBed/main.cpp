#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"

#include <QTextStream>
#include <QFileInfo>

struct Explanation
{
	//supporting transcript, if available
	QByteArray transcript;
	//difference of exon rank in supporting transcript
	int exon_diff;
	//event for left coordinate
	QByteArray event_left;
	//event for right coordinate
	QByteArray event_right;
	//annotations for left coordinate
	QByteArrayList left;
	//annotations for right coordinate
	QByteArrayList right;
};

QDebug operator<<(QDebug debug, const Explanation &expl)
{
    QDebugStateSaver saver(debug);
    debug << expl.transcript << expl.exon_diff << expl.event_left << expl.event_right << expl.left.join(",") << expl.right.join(",");
    return debug;
}

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
		setDescription("Annotates junctions file generated by STAR.");
		addInfile("in", "STAR junctions output file.", false);

		addOutfile("bed", "Output BED file.", true);
		addOutfile("report", "Output annotated junctions as TSV file.", true);
		addOutfile("gene_report", "Output per-gene junction reads as TSV file.", true);

		addInt("min_reads", "Required minimum number of reads spanning a junction.", true, 5);
		addInt("min_overhang", "Required number of overhang bases.", true, 12);

		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020,  6,  24, "Initial version");
	}


	//get exon rank for transcript ID and base position
	int exonRank(NGSD& db, int tx_id, int pos, QByteArray strand="*")
	{
		int exon_rank = -1;
		QString order = (strand == "-") ? "DESC" : "ASC";

		SqlQuery query = db.getQuery();
		query.exec("SET @row_number:=0;");
		query.exec("SELECT txrows.exon_rank FROM ("
							"SELECT @row_number:=@row_number+1 AS exon_rank, gene_exon.* "
							"FROM gene_exon "
							"WHERE transcript_id=" + QString::number(tx_id) + " ORDER BY start " + order + ") AS txrows "
						"WHERE "
							"txrows.start<=" + QString::number(pos) + " AND "
							"txrows.end>=" + QString::number(pos));
		if (query.next())
		{
			exon_rank = query.value(0).toInt();
		}
		return exon_rank;
	}

	//return all exons overlapping specified position, exons are given as gene_symbol:transcript_name:exon_rank
	QMap<QByteArray, int> exons(NGSD& db, Chromosome chr, int pos, QByteArray pos_type="overlap", QByteArray strand="*")
	{
		if (pos_type != "start" && pos_type != "end" && pos_type != "overlap")
		{
			THROW(ArgumentException, "Invalid position type '" + pos_type + "'!");
		}

		QString q = "SELECT gene_transcript.id, gene_transcript.name, gene_transcript.version, gene.symbol "
					"FROM gene_exon, gene_transcript, gene "
					"WHERE "
						"gene_exon.transcript_id=gene_transcript.id AND "
						"gene_transcript.gene_id=gene.id AND "
						"gene_transcript.chromosome='" + chr.strNormalized(false) + "' AND ";
		
		if (pos_type == "overlap")
		{
			q += "gene_exon.start <= " + QString::number(pos) + " AND gene_exon.end >= " + QString::number(pos);
		}
		else
		{
			q += "gene_exon." + pos_type + "=" + QString::number(pos);
		}
						
		if (strand != "*") q.append(" AND gene_transcript.strand='" + strand + "'");

		SqlQuery query = db.getQuery();
		query.exec(q);

		QMap<QByteArray, int> exons;
		while (query.next())
		{
			int tx_id = query.value(0).toInt();
			QByteArray tx_name = query.value(1).toByteArray();
			QByteArray tx_version = query.value(2).toByteArray();
			QByteArray gene_name = query.value(3).toByteArray();
			int exon_rank = exonRank(db, tx_id, pos, strand);

			exons.insert(gene_name + ":" + tx_name + '.' + tx_version, exon_rank);
		}
		return exons;
	}

	//make list of "k:Ev" strings out of map
	QList<QByteArray> mapToKvStr(QMap<QByteArray,int> map)
	{
		QList<QByteArray> out;
		QMap<QByteArray, int>::const_iterator i = map.constBegin();
		while (i != map.constEnd()) {
			out += i.key() + ":E" + QByteArray::number(i.value());
			++i;
		}
		return out;
	}

	//return gene symbols for genes overlapping with given position, strand-aware
	QList<QByteArray> genesByOverlap(NGSD& db, Chromosome chr, int pos, QString strand="*")
	{
		QList<QByteArray> genes;
		SqlQuery query = db.getQuery();

		QString q = "SELECT gene_coord.symbol FROM "
						"(	SELECT gene.symbol, MIN(gene_exon.start) AS start, MAX(gene_exon.end) AS end "
							"FROM gene_exon, gene_transcript, gene "
							"WHERE "
								"gene_exon.transcript_id=gene_transcript.id AND "
								"gene_transcript.gene_id=gene.id AND "
								"gene_transcript.chromosome='" + chr.strNormalized(false) + "' ";
		if (strand != "*") q.append(" AND gene_transcript.strand='" + strand + "' ");
		q +=				"GROUP BY gene.symbol ) as gene_coord "
						"WHERE "
							"gene_coord.start <= " + QString::number(pos) + " AND "
							"gene_coord.end >= " + QString::number(pos);
		query.exec(q);
	
		while (query.next())
		{
			QByteArray gene_name = query.value(0).toByteArray();

			genes.append(gene_name);
		}

		return genes;
	}

	//return string for common transcripts in two sets, with exon numbering
	QMap<QByteArray, int> sharedTranscripts(QMap<QByteArray,int> a, QMap<QByteArray,int> b, QByteArray prefix_a="E", QByteArray prefix_b="E")
	{
		QMap<QByteArray, int> result;
        QSet<QByteArray> shared_transcripts = LIST_TO_SET(a.keys()).intersect(LIST_TO_SET(b.keys()));
		foreach (QByteArray gene_tx, shared_transcripts)
		{
			result.insert(gene_tx + ":" + prefix_a + QByteArray::number(a[gene_tx]) + "-" + prefix_b + QByteArray::number(b[gene_tx]),
							std::abs(a[gene_tx] - b[gene_tx]));
		}

		return result;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		int min_reads = getInt("min_reads");
		int min_overhang = getInt("min_overhang");

		//strand is encoded with 0..2
		QList<QByteArray> strand_map = {"*", "+", "-"};
		//intron motif is encoded with 0..6
		QList<QByteArray> motif_map = {"non-canonical","GT/AG","CT/AC","GC/AG","CT/GC","AT/AC","GT/AT"};

		//BED output file
		QString bed = getOutfile("bed");
		QSharedPointer<QFile> bed_f;
		if (bed != "")
		{	
			bed_f = Helper::openFileForWriting(getOutfile("bed"), true);
			bed_f->write("#gffTags\n");
		}

		//tabular output file
		QString report = getOutfile("report");
		QSharedPointer<QFile> tsv_f;
		if (report != "")
		{
			tsv_f = Helper::openFileForWriting(getOutfile("report"), true);
			QByteArrayList column_headers = QByteArrayList() << "chr" << "intron_start" << "intron_end" << "strand" << "reads" << "motif" << "genes" << "event" << "info";
			tsv_f->write("#" + column_headers.join("\t") + "\n");
		}

		//gene-wise junction read counts, per event
		QString gene_report = getOutfile("gene_report");
		QSharedPointer<QFile> gene_f;
		if (gene_report != "")
		{
			gene_f = Helper::openFileForWriting(getOutfile("gene_report"), true);
		}

		//possible splicing events
		QByteArrayList keys = { "all", "known", "exon-truncation", "exon-skip",
			"exon-skip-truncation", "intronic-or-overlap-no-tx", "other" };

		//count best explanations
		QMap<QByteArray,int> stats;
		foreach (QByteArray k, keys)
		{
			stats[k] = 0;
		}
		//gene-wise statistics
		QMap<QByteArray,QMap<QByteArray,int>> gene_stats;

		auto fp = Helper::openFileForReading(getInfile("in"));
		while (!fp->atEnd())
		{
			QByteArray line = fp->readLine().trimmed();
			if (line.isEmpty()) continue;

			//skip header lines
			if (line.startsWith("#")) continue;

			//split fields
			QByteArrayList parts = line.split('\t');

			//skip special chromosome
			Chromosome chr(parts[0]);
			if (!chr.isNonSpecial()) continue;

			//coverage and alignment criteria
			int unique = Helper::toInt(parts[6]);
			//int multi = Helper::toInt(parts[7]);
			int overhang = Helper::toInt(parts[8]);
			if (unique < min_reads) continue;
			if (overhang < min_overhang) continue;

			//strand
			QByteArray strand = strand_map[Helper::toInt(parts[3])];

			//intron motif
			QByteArray motif = motif_map[Helper::toInt(parts[4])];

			//start and end are first and last intronic bases, use exonic bases
			int intron_start = Helper::toInt(parts[1], "start");
			int intron_end = Helper::toInt(parts[2], "end");
			int exonL_end = intron_start - 1;
			int exonR_start = intron_end + 1;

			//track all explanations
			QMultiMap<QByteArray,QByteArray> expl;
			QMap<int,QList<Explanation>> expl2;
			for (int lvl=0; lvl<=5; ++lvl)
			{
				expl2[0] = QList<Explanation>();
			}

			//exact matches
			QMap<QByteArray, int> matchesL = exons(db, chr, exonL_end, "end", strand);
			QMap<QByteArray, int> matchesR = exons(db, chr, exonR_start, "start", strand);

			//same transcript found for left and right exon
			QMap<QByteArray,int> matchesL_matchesR = sharedTranscripts(matchesL, matchesR);
            foreach (QByteArray gene_tx, SET_TO_LIST(LIST_TO_SET(matchesL_matchesR.keys())))
			{
				int diff = matchesL_matchesR.value(gene_tx);
				if (diff == 1)
				{
					expl2[0].append({gene_tx, diff, "exact match", "exact match", QByteArrayList(), QByteArrayList()});
				}
				else
				{
					expl2[2].append({gene_tx, diff, "exact match", "exact match", QByteArrayList(), QByteArrayList()});
				}
			}

			//both exons known, but no shared transcript
			if (matchesL_matchesR.isEmpty() && !matchesL.isEmpty() && !matchesR.isEmpty())
			{
				expl2[4].append({"", -1, "exact match", "exact match", mapToKvStr(matchesL), mapToKvStr(matchesR)});
			}
			if (matchesL_matchesR.isEmpty())
			{
				//overlapping exons
				QMap<QByteArray,int> overlapsR = exons(db, chr, intron_end, "overlap", strand);
				QMap<QByteArray,int> overlapsL = exons(db, chr, intron_start, "overlap", strand);

				//shared transcripts between matches and overlaps
				QMap<QByteArray,int> matchesL_overlapsR = sharedTranscripts(matchesL, overlapsR);
				QMap<QByteArray,int> overlapsL_matchesR = sharedTranscripts(overlapsL, matchesR);
				QMap<QByteArray,int> overlapsL_overlapsR = sharedTranscripts(overlapsL, overlapsR);

				QList<QByteArray> geneOverlapsL = genesByOverlap(db, chr, intron_start, strand);
				QList<QByteArray> geneOverlapsR = genesByOverlap(db, chr, intron_end, strand);

				//string representations
                QByteArray matchesL_overlapsR_str = SET_TO_LIST(LIST_TO_SET(matchesL_overlapsR.keys())).join(",");
                QByteArray overlapsL_matchesR_str = SET_TO_LIST(LIST_TO_SET(overlapsL_matchesR.keys())).join(",");
                QByteArray overlapsL_overlapsR_str = SET_TO_LIST(LIST_TO_SET(overlapsL_overlapsR.keys())).join(",");
				QByteArray matchesL_str = mapToKvStr(matchesL).join(",");
				QByteArray matchesR_str = mapToKvStr(matchesR).join(",");
				QByteArray overlapsL_str = mapToKvStr(overlapsL).join(",");
				QByteArray overlapsR_str = mapToKvStr(overlapsR).join(",");

				QByteArray geneOverlapsL_str = geneOverlapsL.join(",");
				QByteArray geneOverlapsR_str = geneOverlapsR.join(",");

				if (!matchesL.isEmpty() && matchesR.isEmpty())
				{
					if (!overlapsR.isEmpty() && !matchesL_overlapsR.isEmpty())
					{
                        foreach (QByteArray gene_tx, SET_TO_LIST(LIST_TO_SET(matchesL_overlapsR.keys())))
						{
							int diff = matchesL_overlapsR.value(gene_tx);
							if (diff == 1)
							{
								expl2[1].append({gene_tx, diff, "exact match", "exon overlap", QByteArrayList(), QByteArrayList()});
							}
							else
							{
								expl2[3].append({gene_tx, diff, "exact match", "exon overlap", QByteArrayList(), QByteArrayList()});
							}
						}
					}
					else if (!overlapsR.isEmpty() && matchesL_overlapsR.isEmpty())
					{
						expl2[4].append({"", -1, "exact match", "exon overlap", mapToKvStr(matchesL), mapToKvStr(overlapsR)});
					}
					else if (overlapsR.isEmpty() && !geneOverlapsR.isEmpty())
					{
						expl2[4].append({"", -1, "exact match", "intron overlap", mapToKvStr(matchesL), geneOverlapsR});
					}
					else if (overlapsR.isEmpty() && geneOverlapsR.isEmpty())
					{
						expl2[5].append({"", -1, "exact match", "intergenic", mapToKvStr(matchesL), QByteArrayList()});
					}
				}

				if (!matchesR.isEmpty() && matchesL.isEmpty())
				{
					if (!overlapsL.isEmpty() && !overlapsL_matchesR.isEmpty())
					{
                        foreach (QByteArray gene_tx, SET_TO_LIST(LIST_TO_SET(overlapsL_matchesR.keys())))
						{
							int diff = overlapsL_matchesR.value(gene_tx);
							if (diff == 1)
							{
								expl2[1].append({gene_tx, diff, "exon overlap", "exact match", QByteArrayList(), QByteArrayList()});
							}
							else
							{
								expl2[3].append({gene_tx, diff, "exon overlap", "exact match", QByteArrayList(), QByteArrayList()});
							}
							
						}
					}
					else if (!overlapsL.isEmpty() && overlapsL_matchesR.isEmpty())
					{
						expl2[4].append({"", -1, "exon overlap", "exact match", mapToKvStr(overlapsL), mapToKvStr(matchesR)});
					}
					else if (overlapsL.isEmpty() && !geneOverlapsL.isEmpty())
					{
						expl2[4].append({"", -1, "intron overlap", "exact match", geneOverlapsL, mapToKvStr(matchesR)});
					}
					else if (overlapsL.isEmpty() && geneOverlapsL.isEmpty())
					{
						expl2[5].append({"", -1, "intergenic", "exact match", QByteArrayList(), mapToKvStr(matchesR)});
					}
				}

				if (matchesL.isEmpty() && matchesR.isEmpty())
				{
					if (!overlapsL.isEmpty() && !overlapsR.isEmpty() && !overlapsL_overlapsR.isEmpty())
					{
                        foreach (QByteArray gene_tx, SET_TO_LIST(LIST_TO_SET(overlapsL_overlapsR.keys())))
						{
							int diff = overlapsL_overlapsR.value(gene_tx);
							if (diff == 1)
							{
								expl2[1].append({gene_tx, diff, "exon overlap", "exon overlap", QByteArrayList(), QByteArrayList()});
							}
							else
							{
								expl2[3].append({gene_tx, diff, "exon overlap", "exon overlap", QByteArrayList(), QByteArrayList()});
							}
							
						}
					}
					else if (!overlapsL.isEmpty() && !overlapsR.isEmpty() && overlapsL_overlapsR.isEmpty())
					{
						expl2[4].append({"", -1, "exon overlap", "exon overlap", mapToKvStr(overlapsL), mapToKvStr(overlapsR)});
					}
					else if (!overlapsL.isEmpty() && overlapsR.isEmpty() && !geneOverlapsR.isEmpty())
					{
						expl2[4].append({"", -1, "exon overlap", "intronic", mapToKvStr(overlapsL), geneOverlapsR});
					}
					else if (!overlapsL.isEmpty() && overlapsR.isEmpty() && geneOverlapsR.isEmpty())
					{
						expl2[5].append({"", -1, "exon overlap", "intergenic", mapToKvStr(overlapsL), QByteArrayList()});
					}

					
					else if (!overlapsR.isEmpty() && overlapsL.isEmpty() && !geneOverlapsL.isEmpty())
					{
						expl2[4].append({"", -1, "intronic", "exon overlap", geneOverlapsL, mapToKvStr(overlapsR)});
					}
					else if (!overlapsR.isEmpty() && overlapsL.isEmpty() && geneOverlapsL.isEmpty())
					{
						expl2[5].append({"", -1, "intergenic", "exon overlap", QByteArrayList(), mapToKvStr(overlapsR)});
					}

					else if (overlapsL.isEmpty() && overlapsR.isEmpty())
					{
						if (!geneOverlapsL.isEmpty() && !geneOverlapsR.isEmpty())
						{
							expl2[4].append({"", -1, "intronic", "intronic", geneOverlapsL, geneOverlapsR});
						}
						else if (!geneOverlapsL.isEmpty() && geneOverlapsR.isEmpty())
						{
							expl2[5].append({"", -1, "intronic", "intergenic", geneOverlapsL, QByteArrayList()});
						}
						else if (geneOverlapsL.isEmpty() && !geneOverlapsR.isEmpty())
						{
							expl2[5].append({"", -1, "intergenic", "intronic", QByteArrayList(), geneOverlapsR});
						}
						else if (geneOverlapsL.isEmpty() && geneOverlapsR.isEmpty())
						{
							expl2[5].append({"", -1, "intergenic", "intergenic", QByteArrayList(), QByteArrayList()});
						}

					}
				}
			}

			//use best explanation
			QByteArray event, info;
			QByteArrayList infos;

			// transcript support, perfect match
			if (!expl2[0].isEmpty())
			{
				event = "known";
				foreach (Explanation e, expl2[0]) infos.append(e.transcript);
			}
			// transcript support, exon truncation
			else if (!expl2[1].isEmpty())
			{
				event = "exon-truncation";
				foreach (Explanation e, expl2[1]) infos.append(e.transcript);
			}
			// transcript support, exon skipping
			else if (!expl2[2].isEmpty())
			{
				event = "exon-skip";
				foreach (Explanation e, expl2[2]) infos.append(e.transcript);
			}
			// transcript support, exon truncation + exon skipping
			else if (!expl2[3].isEmpty())
			{
				event = "exon-skip-truncation";
				foreach (Explanation e, expl2[3]) infos.append(e.transcript);
			}
			// no transcript support, intronic
			else if (!expl2[4].isEmpty())
			{
				event = "intronic-or-overlap-no-tx";
				foreach (Explanation e, expl2[4])
					infos.append(e.event_left + "=" + e.left.join("/") + "|" + e.event_right + "=" + e.right.join("/"));
			}
			// no transcript support
			else if (!expl2[5].isEmpty())
			{
				event = "other";
				foreach (Explanation e, expl2[5])
					infos.append(e.event_left + "=" + e.left.join("/") + "|" + e.event_right + "=" + e.right.join("/"));
			}

			info = infos.join(",");

			//extract involved genes
			QSet<QByteArray> genes;
			foreach (QByteArray a, info.split('|'))
			{
				foreach (QByteArray b, a.split(','))
				{
					foreach (QByteArray c, b.split('='))
					{
						QList<QByteArray> d = c.split(':');
						if (d.count() >= 2)
						{
							genes.insert(d[0]);
						}
					}
				}
			}

			QList<QString> namecol = QList<QString>() <<
				"Name=" + VcfFile::encodeInfoValue(event + " (" + QByteArray::number(unique) + " reads)") <<
				"Annotation=" + VcfFile::encodeInfoValue(info) <<
				"Reads=" + QByteArray::number(unique) <<
				"Motif=" + motif;
			QByteArray namecol_str = namecol.join(";").toUtf8();

			//tabular output line
			QByteArrayList out_line = QByteArrayList() <<
				QByteArray(chr.str()) << QByteArray::number(intron_start) << QByteArray::number(intron_end) << strand <<
                QByteArray::number(unique) << motif << genes.values().join(",") << event << info;
			if (report != "") tsv_f->write(out_line.join("\t") + "\n");

			//BED output line
			out_line = QByteArrayList() <<
				QByteArray(chr.str()) <<
				QByteArray::number(intron_start - 1) <<
				QByteArray::number(intron_end) << 
				namecol_str <<
				QByteArray::number(unique) <<
				strand <<
				"." <<
				"." <<
				"224,2,31";
			if (bed != "") bed_f->write(out_line.join("\t") + "\n");
			
			//statistics counter
			++stats[event];
			++stats["all"];

			//junctions reads per gene and type
			foreach (QByteArray gene, genes)
			{
				if (!gene_stats.contains(gene))
				{
					foreach (QByteArray k, keys)
					{
						gene_stats[gene][k] = 0;
					}
				}

				gene_stats[gene][event] += unique;
				gene_stats[gene]["all"] += unique;
			}
		}

		//show stats
		QTextStream out(stdout);
		QMap<QByteArray, int>::const_iterator i = stats.constBegin();
		while (i != stats.constEnd())
		{
            out << i.key() << "\t" << i.value() << "\t" << QByteArray::number(1. * i.value() / stats["all"], 'f', 4) << QT_ENDL;
			++i;
		}

		if (gene_report != "")
		{
			gene_f->write("#symbol\t" + keys.join("\t") + "\taberrant_frac\n");
			//per-gene stats
            foreach (QByteArray g, SET_TO_LIST(LIST_TO_SET(gene_stats.keys())))
			{
				QList<QByteArray> fields;
				fields.append(g);
				foreach(QByteArray k, keys)
				{
					fields.append(QByteArray::number(gene_stats[g][k]));
				}
				fields.append(QByteArray::number(1. * (gene_stats[g]["all"] - gene_stats[g]["known"]) / gene_stats[g]["all"], 'f', 4));
				gene_f->write(fields.join("\t") + "\n");
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
