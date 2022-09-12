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
		addInfileList("in", "Input TSV files that are merged. If only one file is given, each line in this file is interpreted as an input file path.", false);
		addString("cols", "Comma-separated list of column names used as key for merging.", false);
		//optional
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("numeric", "If set, column names are interpreted as 1-based column numbers.");
		addFlag("simple", "Fast and memory-efficient mode for merging files that are ordered in the same way and have no missing lines.");
		addString("mv", "Missing value, i.e. value that is inserted when key is missing in a file.", true, "");

		changeLog(2018, 12 ,  5, "Added 'simple' mode.");
	}

	virtual void main()
	{
		//(1) init
		QStringList in = getInfileList("in");
		if (in.count()==1)
		{
			in = Helper::loadTextFile(in[0], true, '#', true);
		}
		QByteArrayList cols = getString("cols").toUtf8().split(',');
		bool numeric = getFlag("numeric");
		bool simple = getFlag("simple");
		QByteArray missing_value = getString("mv").toUtf8();
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(getOutfile("out"), true);

		if (simple) //simple mode
		{
			QList<QSharedPointer<TSVFileStream>> in_streams;
			QList<QVector<int>> in_cols;

			//open input streams, write comments, determine indices
			foreach(QString file, in)
			{
				//open
				QSharedPointer<TSVFileStream> stream = QSharedPointer<TSVFileStream>(new TSVFileStream(file));
				in_streams << stream;

				//comments
				foreach(const QByteArray& comment_line, stream->comments())
				{
					outstream->write(comment_line + '\n');
				}

				//cols
				in_cols << stream->checkColumns(cols, numeric);
			}

			//write header
			QByteArrayList header_cols;
			for (int s=0; s<in_streams.count(); ++s)
			{
				QSharedPointer<TSVFileStream> stream = in_streams[s];
				const QVector<int>& col_indices = in_cols[s];

				if (s==0)
				{
					foreach(int c, col_indices)
					{
						header_cols << stream->header()[c];
					}
				}
				for(int c=0; c<stream->header().count(); ++c)
				{
					if (col_indices.contains(c)) continue;
					header_cols << stream->header()[c];
				}
			}
			outstream->write('#' + header_cols.join('\t') + '\n');

			//write content
			bool at_end = false;
			while (!at_end)
			{
				QByteArrayList content_cols;
				for (int s=0; s<in_streams.count(); ++s)
				{
					QSharedPointer<TSVFileStream>& stream = in_streams[s];
					if (stream->atEnd())
					{
						at_end = true;
						break;
					}

					const QVector<int>& col_indices = in_cols[s];
					QByteArrayList parts = stream->readLine();

					if (s==0)
					{
						foreach(int c, col_indices)
						{
							content_cols << parts[c];
						}
					}
					else
					{
						int i=0;
						foreach(int c, col_indices)
						{
							if (parts[c]!=content_cols[i])
							{
								THROW(FileParseException, "Mismatch of colum '" + cols[i] + "' in file '" + in[s] + "'. Expected '" + content_cols[i] + "', but found '" + parts[c] + "'!");
							}
							++i;
						}
					}
					for(int p=0; p<parts.count(); ++p)
					{
						if (col_indices.contains(p)) continue;
						content_cols << parts[p];
					}
				}
				if (!at_end)
				{
					outstream->write(content_cols.join('\t') + '\n');
				}
			}

			//check all files have the same line count
			for (int s=0; s<in_streams.count(); ++s)
			{
				QSharedPointer<TSVFileStream> stream = in_streams[s];
				if (!stream->atEnd())
				{
					THROW(FileParseException, "Input file '" + in[s] + "' has more lines than the other files!");
				}
			}

		}
		else //not simple mode
		{
			QByteArrayList comments;
			QByteArrayList header_cols;
			QMap<QByteArray, int> key2index;
			QVector<QByteArrayList> lines;

			//(2) merge data
			foreach(QString file, in)
			{
				//check columns
				TSVFileStream instream(file);
				QVector<int> index_cols = instream.checkColumns(cols, numeric);

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
			//comments
			foreach(const QByteArray& comment_line, comments)
			{
				outstream->write(comment_line + '\n');
			}
			//header
			outstream->write('#' + header_cols.join('\t') + '\n');
			//content
			foreach(const QByteArrayList& line, lines)
			{
				outstream->write(line.join('\t') + '\n');
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

