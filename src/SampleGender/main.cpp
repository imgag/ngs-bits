#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include <QFile>
#include <QTextStream>
#include "Statistics.h"

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
		setDescription("Determines the gender of a sample from the BAM file.");
		addInfile("in", "Input BAM file.", false, true);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		QStringList methods;
		methods << "xy" << "hetx";
		addEnum("method", "Method selection: Read distribution on X and Y chromosome (xy), or fraction of heterocygous variants on X chromosome (hetx).", false, methods);
		addFloat("max_female","Maximum Y/X ratio for female (method xy).", true, 0.06);
		addFloat("min_male","Minimum Y/X ratio for male (method xy).", true, 0.09);
		addFloat("min_female","Minimum heterocygous SNP fraction for female (method hetx).", true, 0.24);
		addFloat("max_male","Maximum heterocygous SNP fraction for male (method hetx).", true, 0.15);
	}

	virtual void main()
	{
		QStringList debug_output;
		QString gender;

		//process
		QString method = getEnum("method");
		if (method=="xy")
		{
			gender = Statistics::genderXY(getInfile("in"), debug_output, getFloat("max_female"), getFloat("min_male"));
		}
		else if (method=="hetx")
		{
			gender = Statistics::genderHetX(getInfile("in"), debug_output, getFloat("max_male"), getFloat("min_female") );
		}

		//output
		QScopedPointer<QFile> outfile(Helper::openFileForWriting(getOutfile("out"), true));
		QTextStream stream(outfile.data());
		foreach(const QString& line, debug_output)
		{
			stream  << line << endl;
		}
		stream << "gender: " << gender << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
