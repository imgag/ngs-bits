#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"
#include <QFileInfo>
#include <QFileInfo>
#include "api/BamReader.h"
#include "Exceptions.h"
#include "Helper.h"
#include "api/BamAlgorithms.h"
#include "NGSHelper.h"

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
		setDescription("Annoates the regions in a BED file with the read count from a BAM file.");
		addInfile("bam", "Input BAM file.", false);
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		//optional
		addInfile("in", "Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
	}

	void readCount(BedFile& bed_file, const QString& bam_file, int min_mapq)
	{
		//check target region is merged/sorted and create index
		if (!bed_file.isMergedAndSorted())
		{
			THROW(ArgumentException, "Merged and sorted BED file required for coverage calculation!");
		}

		//open BAM file
		BamReader reader;
		NGSHelper::openBAM(reader, bam_file);

		//create reference name vector with QString
		QVector<Chromosome> id_to_chr;
		const RefVector& ref_data = reader.GetReferenceData();
		id_to_chr.reserve(ref_data.size());
		for (unsigned int i=0; i<ref_data.size(); ++i)
		{
			id_to_chr.append(Chromosome(ref_data[i].RefName));
		}

		//init coverage statistics data structure
		QVector<long> read_count;
		read_count.fill(0, bed_file.count());

		//iterate through all alignments
		ChromosomalIndex<BedFile> bed_idx(bed_file);
		BamAlignment al;
		while (reader.GetNextAlignmentCore(al))
		{
			if (al.IsDuplicate()) continue;
			if (!al.IsPrimaryAlignment()) continue;
			if (!al.IsMapped() || al.MapQuality<min_mapq) continue;

			const Chromosome& chr = id_to_chr[al.RefID];
			int end_position = al.GetEndPosition();
			QVector<int> indices = bed_idx.matchingIndices(chr, al.Position+1, end_position);
			foreach(int index, indices)
			{
				read_count[index] += 1;
			}
		}
		reader.Close();

		//append readcounts to bed file structure
		for (int i=0; i<bed_file.count(); ++i)
		{
			bed_file[i].annotations().append(QString::number((double)(read_count[i])));
		}
	}

	virtual void main()
	{
		//load and merge regions
		BedFile file;
		file.load(getInfile("in"));
		file.merge(false);

		//get coverage info for bam files
		QString header = "#chr\tstart\tend";
		QString bam = getInfile("bam");
		readCount(file, bam, getInt("min_mapq"));
		header += "\t" + QFileInfo(bam).baseName();

		//store
		file.store(getOutfile("out"), header);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
