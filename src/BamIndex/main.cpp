#include "api/BamReader.h"

#include "ToolBase.h"
#include "Exceptions.h"

using namespace BamTools;

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
        setDescription("Indexes a sorted BAM file.");
        addInfile("in", "Input BAM file.", false, true);
    }

    virtual void main()
    {
		QString bam_file = getInfile("in");

		//open BAM reader
        BamReader reader;
		if (!reader.Open(bam_file.toStdString()))
        {
			THROW(FileAccessException, "Could not open BAM file " + bam_file + ": " + QString::fromStdString(reader.GetErrorString()));
        }

		//create index
		if (!reader.CreateIndex())
        {
			THROW(FileAccessException, "Could not create an index for BAM file " + bam_file + ": " + QString::fromStdString(reader.GetErrorString()));
        }

        reader.Close();
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}
