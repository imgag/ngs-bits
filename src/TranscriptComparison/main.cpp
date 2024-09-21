#include "ToolBase.h"
#include "NGSHelper.h"
#include "TSVFileStream.h"

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
		setDescription("Compares transcripts from RefSeq and Ensembl.");
		addInfile("ensembl", "Ensembl GFF file.", false);
		addInfile("refseq", "RefSeq GFF file.", false);

		//optional
		addOutfile("out", "Output TSV file with matches.", true);
		addFloat("min_ol", "Minum overall/CDS overlap percentage for printing out a relation if there is no perfect match (disabled by default).", true, 100.0);

		//changelog
		changeLog(2024,  9, 20, "First version.");
	}

	struct MatchData
	{
		QByteArray ensembl;
		QByteArray refseq;

		QByteArray gene;
		bool is_coding;

		double ol = -1;
		double ol_cds = -1;
		double ol_utr = -1;

		void write(QSharedPointer<QFile> file, const QByteArray& comment) const
		{
			//output
			QByteArray ol_cds_str = ol_cds==-1 ? "n/a" : QByteArray::number(ol_cds, 'f', 2);
			QByteArray ol_utr_str = ol_utr==-1 ? "n/a" : QByteArray::number(ol_utr, 'f', 2);
			QByteArray coding = is_coding ? "coding" : "non-coding";
			file->write(ensembl + "\t" + refseq + "\t" + gene + "\t" + coding + "\t" + QByteArray::number(ol, 'f', 2) + "\t" + ol_cds_str + "\t" + ol_utr_str + "\t" + comment+ "\n");
			file->flush();
		}
	};

	int printMatches(QSharedPointer<QFile> file, QList<MatchData> matches, bool is_coding, double min_ol)
	{
		int written = 0;

		//coding
		if (is_coding)
		{
			std::sort(matches.begin(), matches.end(), [](const MatchData& a, const MatchData& b){ if (a.ol_cds>b.ol_cds) return true; if (a.ol_cds<b.ol_cds) return false; return a.ol_utr>b.ol_utr; });

			//search for perfect matches first
			foreach(const MatchData& match, matches)
			{
				if (match.ol_cds>=100.0 && match.ol_utr>=100.0)
				{
					match.write(file, "perfect match");
					++written;
				}
			}
			if (written) return written;

			//allow UTR differences (only the first/best)
			double first_match_utr_score = -1.0;
			foreach(const MatchData& match, matches)
			{
				if (match.ol_cds>=100.0)
				{
					if (first_match_utr_score<0) first_match_utr_score = match.ol_utr;
					if (match.ol_utr<first_match_utr_score) continue;
					match.write(file, "perfect CDS match, but UTR differences");
					++written;
				}
			}
			if (written) return written;

			//allow imperfect matches
			foreach(const MatchData& match, matches)
			{
				if (match.ol_cds>=min_ol)
				{
					match.write(file, "above "+QByteArray::number(min_ol, 'f', 2)+"% CDS overlap");
					++written;
				}
			}
		}
		else //non-coding
		{
			std::sort(matches.begin(), matches.end(), [](const MatchData& a, const MatchData& b){ return a.ol>b.ol; });

			//search for perfect matches first
			foreach(const MatchData& match, matches)
			{
				if (match.ol>=100.0)
				{
					match.write(file, "perfect match");
					++written;
				}
			}
			if (written) return written;

			//allow imperfect matches
			foreach(const MatchData& match, matches)
			{
				if (match.ol>=min_ol)
				{
					match.write(file, "above "+QByteArray::number(min_ol, 'f', 2)+"% overlap");
					++written;
				}
			}
		}

		return written;
	}

	virtual void main()
	{
		//init
		QTextStream stream(stdout);
		QTime timer;
		timer.start();
		double min_ol = getFloat("min_ol");

		//load GFFs
		stream << "### loading Ensembl GFF ###" << endl;
		GffSettings s_e;
		TranscriptList trans_e = NGSHelper::loadGffFile(getInfile("ensembl"), s_e).transcripts;
		stream << "took " << Helper::elapsedTime(timer.restart(), true) << endl;
		stream << endl;

		stream << "### loading RefSeq GFF ###" << endl;
		GffSettings s_r;
		s_r.source = "refseq";
		TranscriptList trans_r = NGSHelper::loadGffFile(getInfile("refseq"), s_r).transcripts;
		stream << "took " << Helper::elapsedTime(timer.restart(), true) << endl;
		stream << endl;

		//sort GFFs and create index
		trans_e.sortByPosition();
		trans_r.sortByPosition();
		ChromosomalIndex<TranscriptList> idx_r(trans_r);

		stream << "### comparint transcripts ###" << endl;
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		out->write("##Ensembl file: "+getInfile("ensembl").toLatin1()+"\n");
		out->write("##RefSeq file: "+getInfile("refseq").toLatin1()+"\n");
		out->write("#Ensembl ID\tRefSeq ID\tgene\ttype\toverlap\toverlap_cds\toverlap_utr\tmatch details\n");
		out->flush();

		int written_overall = 0;
		QSet<QByteArray> transcripts_matched;
		QSet<QByteArray> genes;
		QSet<QByteArray> genes_matched;
		foreach (const Transcript& t_e, trans_e)
		{
			QList<MatchData> matches;

			QVector<int> indices = idx_r.matchingIndices(t_e.chr(), t_e.start(), t_e.end());
			foreach(int index, indices)
			{
				const Transcript& t_r = trans_r[index];

				//not on the same strand > skip
				if	(t_e.strand()!=t_r.strand()) continue;

				//not matching coding status > skip
				if	(t_e.isCoding()!=t_r.isCoding()) continue;

				//no name (chrMT genes) > skip
				if (t_r.name().isEmpty()) continue;

				//prepare match data
				MatchData data;
				data.ensembl = t_e.name();
				data.refseq = t_r.name();
				data.gene = t_e.gene();
				data.is_coding = t_e.isCoding();

				//count genes
				genes << t_e.gene();

				//calculate overall overlap
				BedFile region = t_e.regions();
				double bases_ens = region.baseCount();
				region.intersect(t_r.regions());
				double bases_ref = t_r.regions().baseCount();
				data.ol = 100.0 * region.baseCount() / std::max(bases_ens, bases_ref);

				if (t_e.isCoding())
				{
					//calculate CDS overlap
					region = t_e.codingRegions();
					bases_ens = region.baseCount();
					region.intersect(t_r.codingRegions());
					bases_ref = t_r.codingRegions().baseCount();
					data.ol_cds = 100.0 * region.baseCount() / std::max(bases_ens, bases_ref);

					//calculate UTR overlap
					region = t_e.utr3prime();
					region.add(t_e.utr5prime());
					region.sort();
					bases_ens = region.baseCount();
					BedFile region2 = t_r.utr3prime();
					region2.add(t_r.utr5prime());
					region2.sort();
					bases_ref = region2.baseCount();
					region.intersect(region2);
					double max_bases = std::max(bases_ens, bases_ref);
					data.ol_utr = max_bases==0 ? 100.0 : 100.0 * region.baseCount() / max_bases;
				}

				matches.append(data);
			}

			//select matches to report
			int written = printMatches(out, matches, t_e.isCoding(), min_ol);
			written_overall += written;
			if (written>0)
			{
				transcripts_matched << t_e.name();
				genes_matched << t_e.gene();
			}
		}

		stream << "Overall transcript matches written: " << written_overall << endl;
		stream << "Transcripts with match: " << transcripts_matched.count() << endl;
		stream << "Transcripts without match: " << (trans_e.count()-transcripts_matched.count()) << endl;
		stream << "Genes with match: " << genes_matched.count() << endl;
		stream << "Genes without match: " << (genes.count()-genes_matched.count()) << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
