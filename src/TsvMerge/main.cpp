#include "ToolBase.h"
#include "TSVFileStream.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QFileInfo>
#include <QBitArray>

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
		setDescription("Merges TSV file based on a list of columns.");
		addInfileList("in", "Input TSV files that are merged.", false);
		addString("cols", "Comma-separated list of column names used as key for merging.", false);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("numeric", "If set, column names are interpreted as 1-based column numbers.");
		addString("mv", "Missing value, i.e. value that is inserted when key is missing in a file.", true, "");
	}

	virtual void main()
	{
		//(1) init
		QByteArrayList comments;
		QByteArrayList header_cols;
		QMap<QByteArray, int> key2index;
		QVector<QByteArrayList> lines;
		QByteArray missing_value = getString("mv").toLatin1();

		//(2) merge data
		QStringList in = getInfileList("in");
		foreach(QString file, in)
		{
			//check columns
			TSVFileStream instream(file);
			QVector<int> index_cols = instream.checkColumns(getString("cols"), getFlag("numeric"));

			//init headers with key columns (first file only)
			if (header_cols.isEmpty())
			{
				foreach(int i, index_cols)
				{
					header_cols << instream.header()[i];
				}
			}

			//add comments
			comments << instream.comments();

			//add column headers of current file
			const int cols_before_add = header_cols.count();
			QVector<int> non_index_cols;
			for(int i=0; i<instream.header().count(); ++i)
			{
				if (index_cols.contains(i)) continue;
				header_cols << instream.header()[i];
				non_index_cols.append(i);
			}
			const int cols_after_add = header_cols.count();

			//add content
			while(!instream.atEnd())
			{
				QByteArrayList parts = instream.readLine();

				//construct key
				QByteArray key;
				foreach(int i, index_cols)
				{
					if(!key.isEmpty())
					{
						key.append('\t');
					}
					key.append(parts[i]);
				}

				//determine key index
				int key_index = key2index.value(key, -1);

				//unknown key => append to key list and append a new line
				if (key_index==-1)
				{
					key_index = lines.count();
					key2index[key] = key_index;

					QByteArrayList cols;
					foreach(int i, index_cols)
					{
						cols.append(parts[i]);
					}
					while (cols.count()<cols_before_add)
					{
						cols.append(missing_value);
					}
					lines.append(cols);
				}

				//append data to line
				QByteArrayList& line = lines[key_index];
				foreach(int i, non_index_cols)
				{
					line.append(parts[i]);
				}
			}

			//add content for missing keys
			foreach(int key_index, key2index)
			{
				QByteArrayList& line = lines[key_index];
				while(line.count()<cols_after_add)
				{
					line.append(missing_value);
				}
			}
		}

		//(3) write output
		QString out = getOutfile("out");
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, true);
		//comments
		outstream->write(comments.join('\n')+ '\n');
		//header
		outstream->write('#' + header_cols.join('\t') + '\n');
		//content
		foreach(const QByteArrayList& line, lines)
		{
			outstream->write(line.join('\t') + '\n');
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

