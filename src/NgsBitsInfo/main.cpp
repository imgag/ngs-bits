#include "ToolBase.h"
#include <htslib/tbx.h>
#include <QSysInfo>
#include <QLibraryInfo>

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
		setDescription("Writes general information about ngs-bits to STDOUT.");

		changeLog(2026,  6, 18, "Initial version.");
	}

	virtual void main()
	{
		QTextStream out(stdout);
		out << "ngs-bits version: " << ToolBase::version() << "\n";
		out << "ngs-bits date: " << ToolBase::date() << "\n";
		out << "operating system: " + QSysInfo::prettyProductName() << "\n";
		out << "architecture: " + QSysInfo::buildCpuArchitecture() << "\n";
		out << "Qt version: " + QLibraryInfo::version().toString() << "\n";
		out << "htslib version: " << hts_version() << "\n";
		out << Qt::endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
    return tool.execute();
}
