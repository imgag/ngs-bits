#include "BedFile.h"
#include "ToolBase.h"
#include "Exceptions.h"
#include "ChromosomalFileIndex.h"
#include "Helper.h"
#include <QRegExp>
#include <QTextStream>
#include <QFile>

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
		setDescription("Indexes and searches tab-separated chromosomal database files (1-based positions).");
		addInfile("in", "Input database file.", false, true);
		addInfile("bed", "Chromosomal ranges file for which the database content is retrieved.", true, true);
		addString("pos", "Chromosomal range in format 'chr:start-end' for which the database content is retrieved.", true, "");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("force", "Forces database index update, even if it is up-to-date.");
		addInt("c", "Database file 0-based chromosome column index (Only needed to create index file).", true, 0);
		addInt("s", "Database file 0-based start position column index (Only needed to create index file).", true, 1);
		addInt("e", "Database file 0-based end position column index (Only needed to create index file).", true, 2);
		addString("h", "Database file header comment character (Only needed to create index file).", true, "#");
		addInt("b", "Index bin size: An index entry is created for every n'th line in the database (Only needed to create index file).", true, 1000);
	}

	virtual void main()
	{
		//parameter error checks
		QString header = getString("h");
		if (header.length()!=1)
		{
			THROW(CommandLineParsingException, "Invalid value '" + header + "' for header comment character. The parameter argument has to be one character!");
		}

		QString pos = getString("pos");
		if (pos!="" && !QRegExp("[A-Za-z0-9_]+:[0-9]+-[0-9]+").exactMatch(pos))
		{
			THROW(CommandLineParsingException, "Invalid chromosomal position '" + pos + "'!");
		}

		QString bed = getInfile("bed");
		if (bed=="" && pos=="")
		{
			THROW(CommandLineParsingException, "You have to provide a chromosomal range using the parameters 'pos' or 'bed'!");
		}
		if (bed!="" && pos!="")
		{
			THROW(CommandLineParsingException, "You can provide only one of the parameters 'pos' and 'bed' at a time!");
		}

		//open output stream
		QScopedPointer<QFile> outfile(Helper::openFileForWriting(getOutfile("out"), true));
		QTextStream out(outfile.data());

		//create/load index
		QString filename = getInfile("in");
		ChromosomalFileIndex index;
		if (getFlag("force") || !ChromosomalFileIndex::isUpToDate(filename))
		{
			index.create(filename, getInt("c"), getInt("s"), getInt("e"), header[0].toLatin1(), getInt("b"));
			QTextStream(stdout, QFile::WriteOnly) << "Note: Database index updated!" << endl;
		}
		index.load(filename);

		//Search using position string
		if (pos!="")
		{
			QStringList parts = QString(pos).replace('-',':').split(':');
			QStringList db_lines = index.lines(parts[0], parts[1].toInt(), parts[2].toInt());
			foreach(const QString& db_line, db_lines)
			{
				out << pos << '\t' << db_line << endl;
			}
		}

		//Search using BED file
		if (bed!="")
		{
			BedFile bed_file;
			bed_file.load(bed);
			for (int i=0; i<bed_file.count(); ++i)
			{
				const BedLine& line = bed_file[i];
				QStringList db_lines = index.lines(line.chr(), line.start(), line.end());
				foreach(const QString& db_line, db_lines)
				{
					out << line.chr().str() << ':' << line.start() << '-' << line.end() << '\t' << db_line << endl;
				}
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
