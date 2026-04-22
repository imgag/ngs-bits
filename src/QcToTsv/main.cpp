#include "ToolBase.h"
#include "QCCollection.h"
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
		setDescription("Converts qcML files to a TSV file..");
		addInfileList("in", "Input qcML files.", false);
		//optional
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true, true);
		addInfile("obo", "OBO file to use. If unset, uses the default file compiled into ngs-bits.", true, true);

		changeLog(2025,  1, 10, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QString obo = getInfile("obo");
		if (obo=="") obo = "://Resources/qcML.obo";

		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		out->write("#accession\tname\tvalue\n");

		//process
		foreach(QString file, getInfileList("in"))
		{
			QStringList errors;
			QCCollection qc = QCCollection::fromQCML(file, obo, errors);
			for(int i=0; i<qc.count(); ++i)
			{
				if (qc[i].type()==QCValueType::IMAGE) continue;

				out->write((qc[i].accession() + "\t" + qc[i].name() + "\t" + qc[i].toString() + "\n").toUtf8());
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

