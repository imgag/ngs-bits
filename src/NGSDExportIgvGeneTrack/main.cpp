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
		setDescription("Writes all transcripts and exons of all genes to a IGV-readable text file.");
		addOutfile("out", "The output text file.", false);
		addOutfile("out_mane", "The optional output text file containing only MANE + clinical transcripts.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
	}


    virtual void main()
    {
        // open output file
        QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		bool export_mane = !getOutfile("out_mane").isEmpty();
		QSharedPointer<QFile> outfile_mane;
		if (export_mane) outfile_mane = Helper::openFileForWriting(getOutfile("out_mane"), true);

        NGSD db(getFlag("test"));

        const TranscriptList transcripts = db.transcripts();
		const auto matches = NGSHelper::transcriptMatches(GenomeBuild::HG38);


        foreach(const Transcript& trans, transcripts)
        {
			QByteArray name = trans.name();
			if (matches[name].size() > 0) name += ", " + matches[name].join(", ");

			const QByteArray strand = trans.strandToString(trans.strand());
			int phase = -1;
			//calculate codon offset and cds start/end
			QByteArray cds_start, cds_end;
			if (strand == "+")
			{
//				phase = (trans.codingStart()-trans.start()) % 3;
				cds_start = (trans.codingStart() > 0)?QByteArray::number(trans.codingStart()-1):QByteArray::number(trans.end());
				cds_end = (trans.codingEnd() > 0)?QByteArray::number(trans.codingEnd()):QByteArray::number(trans.end());
			}
			else if (strand == "-")
			{
//				phase =(trans.end()-trans.codingStart()) % 3;
				//write in chr order
				cds_end = (trans.codingStart() > 0)?QByteArray::number(trans.codingStart()):QByteArray::number(trans.end());
				cds_start = (trans.codingEnd() > 0)?QByteArray::number(trans.codingEnd()-1):QByteArray::number(trans.end());
			}


            const BedFile& coding_regions = trans.codingRegions();
			QByteArray cds_status = "none";
			QList<QList<int>> exon_ranges;

            if (!coding_regions.isEmpty()) {
                const BedFile& utr3prime = trans.utr3prime();
                for ( int i=0; i<utr3prime.count(); ++i )
                {
                    // 3prime UTR LINEs
                    const BedLine& reg = utr3prime[i];
					//add (exon) start, end and frame offset
					exon_ranges.append(QList<int>() << reg.start() << reg.end() << -1);
                }

				int cds_offset = 0;
				for ( int i=0; i<coding_regions.count(); ++i )
				{
					// CDS LINEs
					// for transcripts on the reverse strand: start with the last exon and run backwards
					const BedLine& coding_region = (trans.strand() == Transcript::MINUS)? coding_regions[(coding_regions.count()-1)-i]: coding_regions[i];
					//save current offset
					phase = cds_offset;
					//determine new offset:
					cds_offset = (cds_offset + coding_region.length()) % 3;

					//update cds status
					cds_status = "cmpl";

//					if (((trans.strand()==Transcript::MINUS && i==0) || (trans.strand()==Transcript::PLUS && i==(coding_regions.count()-1))) && exon_ranges.size()>0)
//					{
//						//on first iteration: check if utr and exon are connected
//						if (exon_ranges.last()[1] == coding_region.start())
//						{
//							exon_ranges.last()[1] = coding_region.end();
//							exon_ranges.last()[2] = phase;
//							continue;
//						}
//					}

					//add (exon) start, end and frame offset
					exon_ranges.append(QList<int>() << coding_region.start() << coding_region.end() << phase);



				}

                const BedFile& utr5prime = trans.utr5prime();
                for ( int i=0; i<utr5prime.count(); ++i )
                {
                    // 5prime UTR LINEs
                    const BedLine& reg = utr5prime[i];

//					if (((trans.strand()==Transcript::MINUS && i==0) || (trans.strand()==Transcript::PLUS && i==(coding_regions.count()-1))) && exon_ranges.size()>0)
//					{
//						//on first iteration: check if utr and exon are connected
//						if (exon_ranges.last()[1] == (reg.start()-1))
//						{
//							exon_ranges.last()[1] = reg.end();
//							continue;
//						}
//					}
					//add (exon) start, end and frame offset
					exon_ranges.append(QList<int>() << reg.start() << reg.end() << -1);
                }
            } else {
                const BedFile& exons = trans.regions();
                for ( int i=0; i<exons.count(); ++i )
                {
                    // 5prime UTR LINEs
                    const BedLine& reg = exons[i];
					//add (exon) start, end and frame offset
					exon_ranges.append(QList<int>() << reg.start() << reg.end() << -1);
                }
            }

			//sort exons by coordinate
			std::sort(exon_ranges.begin(), exon_ranges.end(),[](const QList<int>& a, const QList<int>& b)->bool{return a.at(0)<b.at(0);});

			//merge regions (and convert to 0-based)
			QList<QList<int>> merged_exon_ranges;
			foreach(QList<int> exon, exon_ranges)
			{
				if (merged_exon_ranges.size() > 0)
				{
					if (merged_exon_ranges.last()[1] == (exon[0]-1))
					{
						merged_exon_ranges.last()[1] = exon[1];
						merged_exon_ranges.last()[2] = std::max(merged_exon_ranges.last()[2], exon[2]);
						continue;
					}
				}
				exon[0] -= 1; //convert to 0-based
				merged_exon_ranges.append(exon);
			}


			//format for columns
			QByteArray exon_starts, exon_ends, exon_frames;
			foreach(const QList<int>& exon, merged_exon_ranges)
			{
				exon_starts += QByteArray::number(exon.at(0)) + ",";
				exon_ends += QByteArray::number(exon.at(1)) + ",";
				exon_frames += QByteArray::number(exon.at(2)) + ",";
			}


			//write line
			QByteArrayList line;
			line.append("0"); //bin
			line.append(name);
			line.append(trans.chr().strNormalized(true));
			line.append(strand);
			line.append(QByteArray::number(trans.start()-1)); // -1 since it is 0-based
			line.append(QByteArray::number(trans.end()));
			line.append(cds_start);
			line.append(cds_end);
			line.append(QByteArray::number(merged_exon_ranges.size()));
			line.append(exon_starts);
			line.append(exon_ends);
			line.append("0"); //score
			line.append(trans.gene());
			line.append(cds_status);
			line.append(cds_status);
			line.append(exon_frames);

			outfile->write(line.join('\t') + "\n");
			if (trans.isManePlusClinicalTranscript() || trans.isManeSelectTranscript()) outfile_mane->write(line.join('\t') + "\n");
		}


    }

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
