#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"
#include <QFileInfo>
#include <QFileInfo>
#include "Exceptions.h"
#include "Helper.h"
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
		setDescription("Annotates the regions in a BED file with the read count from a BAM/CRAM file.");
		addInfile("bam", "Input BAM/CRAM file.", false);
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		//optional
		addInfile("in", "Input BED file (note that overlapping regions will be merged before processing). If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2020,  11, 27, "Added CRAM support.");
	}

	void readCount(BedFile& bed_file, const QString& bam_file, int min_mapq, const QString& ref_file)
	{
		//check target region is merged/sorted and create index
		if (!bed_file.isMergedAndSorted())
		{
			THROW(ArgumentException, "Merged and sorted BED file required for coverage calculation!");
		}

		//open BAM file
		BamReader reader(bam_file, ref_file);
		reader.skipBases();
		reader.skipQualities();
		reader.skipTags();

		//init coverage statistics data structure
		QVector<qlonglong> read_count;
		read_count.fill(0, bed_file.count());

		//iterate through all alignments
		ChromosomalIndex<BedFile> bed_idx(bed_file);
		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			if (al.isUnmapped()) continue;
			if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
			if (al.isUnmapped() || al.mappingQuality()<min_mapq) continue;

			QVector<int> indices = bed_idx.matchingIndices(reader.chromosome(al.chromosomeID()), al.start(), al.end());
			foreach(int index, indices)
			{
				read_count[index] += 1;
			}
		}

		//append readcounts to bed file structure
		for (int i=0; i<bed_file.count(); ++i)
		{
			bed_file[i].annotations().append(QByteArray::number(read_count[i]));
		}
	}

	virtual void main()
	{
		//load and merge regions
		BedFile file;
		file.load(getInfile("in"));
		file.merge(false);

		//get coverage info for bam files
		QString bam = getInfile("bam");
		readCount(file, bam, getInt("min_mapq"), getInfile("ref"));

		//store
		file.clearHeaders();
		file.appendHeader("#chr\tstart\tend\t" + QFileInfo(bam).baseName().toUtf8());
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
