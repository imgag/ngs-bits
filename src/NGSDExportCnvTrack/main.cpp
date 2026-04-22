#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "Exceptions.h"
#include "BasicStatistics.h"
#include <QBitArray>

struct CNV
{
	int start;
	int end;
	int cn;
};

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
		setDescription("Exports a IGV-conform CNV track for a processing system.");
		addString("system", "Processing system name filter (short name).", true, "");
		addOutfile("out", "Output IGV file.", false);
		//optional
		addFloat("min_dp", "Minimum depth of the processed sample.", true, 0.0);
		addFloat("max_cnvs", "Maximum number of CNVs per sample.", true, 0.0);
		addFloat("min_af", "Minimum allele frequency of output CNV ranges.", true, 0.01);
		addString("caller_version", "Restrict output to callsets with this caller version.", true, "");
		addOutfile("stats", "Statistics and logging output. If unset, writes to STDOUT", true);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2019, 10, 21, "First version");
	}

	static bool nextOverlappingRange(const QList<CNV> cnvs, int& i_start, int& i_end)
	{
		const int cnv_count = cnvs.size();
		if (i_end+1 >= cnv_count) return false;

		//init
		i_start = i_end+1;
		i_end = i_end+1;
		const int start = cnvs[i_start].start;
		int end = cnvs[i_end].end;

		//move until end of overlap range
		while(i_end+1 < cnv_count && BasicStatistics::rangeOverlaps(start, end, cnvs[i_end+1].start, cnvs[i_end+1].end))
		{
			++i_end;
			end = std::max(end, cnvs[i_end].end);
		}

		return true;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString system = getString("system");
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"));
		QTextStream stream(out.data());
		QSharedPointer<QFile> stats = Helper::openFileForWriting(getOutfile("stats"), true);
		QTextStream stream2(stats.data());
		double min_dp = getFloat("min_dp");
		double max_cnvs = getFloat("max_cnvs");
		double min_af = getFloat("min_af");
		if (max_cnvs==0.0) max_cnvs = std::numeric_limits<double>::max();
		QString caller_version = getString("caller_version");

		//check that system is valid
		int sys_id = db.processingSystemId(system, false);
		if (sys_id==-1)
		{
			THROW(DatabaseException, "Invalid processing system short name '" + system + "'.\nValid names are: " + db.getValues("SELECT name_short FROM processing_system ORDER BY name_short ASC").join(", "));
		}

		//read data
		QVector<double> stats_cnvs;
		QVector<double> stats_depth;
		QStringList cs_ids =  db.getValues("SELECT cs.id FROM cnv_callset cs, processed_sample ps WHERE ps.processing_system_id=" + QString::number(sys_id) + " AND ps.id=cs.processed_sample_id AND ps.quality!='bad' AND cs.quality!='bad'");
		stream2 << "Found " << cs_ids.count() << " high-quality CNV callsets for the processing system.\n";
		stream2.flush();
		QBitArray skip(cs_ids.size(), false);
		for (int i=0; i<cs_ids.count(); ++i)
		{
			const QString& cs_id = cs_ids[i];

			//processed sample name
			QString ps = db.processedSampleName(db.getValue("SELECT processed_sample_id FROM cnv_callset WHERE id='" + cs_id + "'").toString());

			//depth
			QVariant depth = db.getValue("SELECT qc.value FROM processed_sample_qc qc, qc_terms t, cnv_callset cs WHERE t.id=qc.qc_terms_id AND t.qcml_id='QC:2000025' AND cs.processed_sample_id=qc.processed_sample_id AND cs.id='" + cs_id + "'");
			if (!depth.isNull())
			{
				bool ok = false;
				double depth_val = depth.toDouble(&ok);
				if (ok)
				{
					if (depth_val<min_dp)
					{
						stream2 << "Skipping sample " << ps << " - depth (" << depth_val << ") is below " << min_dp << "!\n";
						skip[i] = true;
						continue;
					}
					stats_depth << depth_val;
				}
			}

			//CNV count
			int cnv_count = db.getValue("SELECT count(*) FROM cnv WHERE cnv_callset_id=" + cs_id).toInt();
			if (cnv_count>max_cnvs)
			{
				stream2 << "Skipping sample " << ps << " - CNV count (" << cnv_count << ") is higher than " << max_cnvs << "!\n";
				skip[i] = true;
				continue;
			}
			
			//caller version
			if (caller_version!="")
			{
				QString version = db.getValue("SELECT caller_version FROM cnv_callset WHERE id=" + cs_id).toString();
				if(version!=caller_version)
				{
					stream2 << "Skipping sample " << ps << " - caller version (" << version << ") is wrong!\n";
					skip[i] = true;	
				}
			}
			
			stats_cnvs  << cnv_count;
		}
		const int sample_count = skip.count(false);
		stream2 << "Using " << sample_count << " of " << cs_ids.count() << " callsets\n";
		stream2.flush();

		//write stats
		stream2 << "Statistics - number of CNVs\n";
		if (stats_cnvs.isEmpty())
		{
			stream2 << "  no callsets!\n";
		}
		else
		{
			std::sort(stats_cnvs.begin(), stats_cnvs.end());
			stream2 << "  min   : " << stats_cnvs.first() << "\n";
			stream2 << "  q1    : " << BasicStatistics::q1(stats_cnvs, false) << "\n";
			stream2 << "  median: " << BasicStatistics::median(stats_cnvs, false) << "\n";
			stream2 << "  q3    : " << BasicStatistics::q3(stats_cnvs, false) << "\n";
			stream2 << "  max   : " << stats_cnvs.last() << "\n";
		}

		stream2 << "Statistics - depth\n";
		if (stats_depth.isEmpty())
		{
			stream2 << "  no callsets!\n";
		}
		else
		{
			std::sort(stats_depth.begin(), stats_depth.end());
			stream2 << "  min   : " << stats_depth.first() << "\n";
			stream2 << "  q1    : " << BasicStatistics::q1(stats_depth, false) << "\n";
			stream2 << "  median: " << BasicStatistics::median(stats_depth, false) << "\n";
			stream2 << "  q3    : " << BasicStatistics::q3(stats_depth, false) << "\n";
			stream2 << "  max   : " << stats_depth.last() << "\n";
		}
		stream2.flush();

		//process chr by chr
		stream << "#track graphtype=heatmap viewLimits=0.0:1.0 color=0,0,255 altColor=255,255,255 midRange=0.001:0.02 midColor=204,204,255 windowingFunction=maximum\n";
		stream << "Chromosome\tStart\tEnd\tCN histogram (0-10)\tAF " << system << "\n";
		SqlQuery q_cnvs = db.getQuery();
		q_cnvs.prepare("SELECT start, end, cn FROM cnv WHERE cnv_callset_id=:0 AND chr=:1");

		QStringList chrs = db.getEnum("cnv", "chr");
		foreach(const QString& chr, chrs)
		{
			stream2 << "Processing chromosome " << chr << "...\n";
			stream2.flush();

			//load all CNVs of the chromosome
			QList<CNV> cnvs;
			for (int i=0; i<cs_ids.count(); ++i)
			{
				if (skip[i]) continue;

				const QString& cs_id = cs_ids[i];

				q_cnvs.bindValue(0, cs_id);
				q_cnvs.bindValue(1, chr);
				q_cnvs.exec();
				while(q_cnvs.next())
				{
					cnvs << CNV {q_cnvs.value(0).toInt(), q_cnvs.value(1).toInt()-1, q_cnvs.value(2).toInt()}; //subtract 1 to remove one-base overlaps
				}
			}
			stream2 << "  Found " << cnvs.count() << " CNVs\n";
			stream2.flush();

			//sort by pos
			std::sort(cnvs.begin(), cnvs.end(), [](const CNV& a, const CNV& b){ return a.start < b.start;});

			//merge infos
			int i_start = -1;
			int i_end = -1;
			while (nextOverlappingRange(cnvs, i_start, i_end))
			{
				//determine all sub-regions of the overlapping range
				QList<int> positions;
				for (int i=i_start; i<=i_end; ++i)
				{
					positions << cnvs[i].start;
					positions << cnvs[i].end+1;
				}
				std::sort(positions.begin(), positions.end());
				positions.erase(std::unique(positions.begin(), positions.end()), positions.end() );

				//process all sub-regions
				for (int i=0; i<positions.size()-1; ++i)
				{
					const int start = positions[i];
					const int end = positions[i+1]-1;

					//count matches (AF) and create CN histogram
					QVector<int> cn_hist(10, 0);
					int matches = 0;
					for (int i=i_start; i<=i_end; ++i)
					{
						if (BasicStatistics::rangeOverlaps(start, end, cnvs[i].start, cnvs[i].end))
						{
							++matches;

							int cn = BasicStatistics::bound(cnvs[i].cn, 0, 9);
							++cn_hist[cn];
						}
					}

					//output
					double af = (double)matches/sample_count;
					if (af>=min_af)
					{
						stream << chr << "\t" << start << "\t" << (end+1) << "\t";
						for (int i=0; i<cn_hist.size(); ++i)
						{
							if (i>0) stream << ',';
							stream << (i==2 ? sample_count-matches : cn_hist[i]);
						}
						stream << "\t" << QString::number(af, 'f', 4) << "\n";
					}
				}
			}
		}

		//cleanup
		stats->close();
		out->close();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
