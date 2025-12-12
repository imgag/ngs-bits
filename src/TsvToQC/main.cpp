#include "ToolBase.h"
#include "Helper.h"
#include "QCCollection.h"
#include "OntologyTermCollection.h"

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
		setDescription("Converts TSV file to a qcML file.");

		addInfileList("sources", "Source files the QC terms were extracted from.", false);
		//optional
		addInfile("in", "Input TSV file with two columns (QC term accession and value). If unset, reads from STDIN.", true);
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);

		changeLog(2022,  9, 20, "Initial inplementation.");
	}

	virtual void main()
	{
		//load qcML terms
		OntologyTermCollection terms("://Resources/qcML.obo", false);

		//parse input and generate output
		QCCollection output;
		QSharedPointer<QFile> in = Helper::openFileForReading(getInfile("in"), true);
		while(!in->atEnd())
		{
			QByteArray line = in->readLine().trimmed();
			if (line.isEmpty() || line.startsWith('#')) continue;

			QByteArrayList parts = line.split('\t');
			if (parts.count()!=2) THROW(FileParseException, "Input line with more/less than two parts: '" + line + "'");

			QByteArray accession = parts[0];
			QByteArray value = parts[1];

			//check term accession exists
			if (!terms.containsByID(accession))
			{
				THROW(ProgrammingException, "qcML OBO file does not contain term with accession '" + accession + "'!");
			}
			const OntologyTerm& term = terms.getByID(accession);

			output.insert(QCValue(term.name(), QString(value), term.definition(), accession));
		}
		in->close();

		//write output
		output.storeToQCML(getOutfile("out"), getInfileList("sources"), "");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

