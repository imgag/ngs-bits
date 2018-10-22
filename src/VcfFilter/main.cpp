#include "ToolBase.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include <QFile>

class ConcreteTool: public ToolBase
{
    Q_OBJECT

public:
    ConcreteTool(int& argc, char *argv[])
        : ToolBase(argc, argv)
    {
    }

    virtual void setup()
    {
		setDescription("Filters a VCF based on the given criteria.");
        //optional
        addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addString("reg", "Region of interest in BED format, or comma-separated list of region, e.g. 'chr1:454540-454678,chr2:473457-4734990'.", true, true);
        addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		//TODO reg: BED/comma-separated regions
		//TODO quality:min
		//TODO filter_empty ('PASS', '', '.')
		//TODO filter:regexp
		//TODO id:regexp
		//TODO variant_type: SNV,INDEL,OTHER > addEnum();
		//TODO info:complex e.g. 'AO > 2;DP > 10' (==,>=,<=,<,>,!=, contains,is)
		//TODO sample: complex

		changeLog(2018, 10, 31, "Initial implementation.");
    }

    virtual void main()
    {
		//init
		QString reg = getInfile("reg");

		//load target region
		BedFile roi;
		if (QFile::exists(reg))
		{
			roi.load(reg);
		}
		else //parse comma-separated regions
		{
			//TODO
		}
		roi.merge();
		ChromosomalIndex roi_index(roi);

        //open input/output streams
        QString in = getInfile("in");
        QString out = getOutfile("out");
        if(in!="" && in==out)
        {
            THROW(ArgumentException, "Input and output files must be different when streaming!");
        }
        QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
        QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

        while(!in_p->atEnd())
        {
            QByteArray line = in_p->readLine();

            //skip empty lines
            if (line.trimmed().isEmpty()) continue;

			//header
            if (line.startsWith('#'))
			{
				out_p->write(line);
                continue;
            }

			//filter roi
			if (reg!="" && !roi_index.matchingIndex(chr, start, end)) continue;


			out_p->write(line);
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
