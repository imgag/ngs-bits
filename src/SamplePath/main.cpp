#include "ToolBase.h"
#include "NGSD.h"

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	QStringList validPathStrings()
	{
		QStringList output;

		foreach(PathType type, QList<PathType>() << PathType::SAMPLE_FOLDER << PathType::BAM << PathType::VCF << PathType::GSVAR << PathType::COPY_NUMBER_CALLS << PathType::STRUCTURAL_VARIANTS)
		{
			output << FileLocation::typeToString(type);
		}

		return output;
	}

	virtual void setup()
	{
		setDescription("Prints the folder of a processed sample.");
		addString("ps", "Processed sample name.", false);
		//optional
		addEnum("type", "Path type to print.", true, validPathStrings(), "SAMPLE_FOLDER");
		addFlag("test", "Uses the test database instead of on the production database.");

		//changelog
		changeLog(2023, 7, 18, "Initial version");
	}

	virtual void main()
	{
		NGSD db(getFlag("test"));

		QString ps = getString("ps");
		QString ps_id = db.processedSampleId(ps);

		QString type = getEnum("type");

		QTextStream(stdout) << db.processedSamplePath(ps_id, FileLocation::stringToType(type)) << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
