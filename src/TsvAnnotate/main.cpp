#include "ToolBase.h"
#include "TSVFileStream.h"
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
		setDescription("Extends TSV file by appending columns from a second TSV file.");
		addInfile("in2", "Input TSV files that is used as source of annotated columns.", false);
		addString("c1", "Column in 'in1' that is used for matching lines between files.", false);
		addString("anno", "Comma-separated column list from 'in2' that is appended to 'in1'. Order matters.", false);
		//optional
		addInfile("in1", "Input TSV files that is annoated. If unset, reads from STDIN.", true);
		addString("c2", "Column in 'in2' that is used for matching lines between files. If unset, the value of 'c1' is used.", true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addString("mv", "Missing value, i.e. value that is used when data is missing in 'in2'.", true, "");

		changeLog(2026,  1, 20, "First version.");
	}

	virtual void main()
	{
		//init
		QByteArrayList anno = getString("anno").toUtf8().split(',');
		QByteArray missing_value = getString("mv").toUtf8();
		missing_value = QByteArrayList(anno.count(), missing_value).join('\t');
		QByteArray c1 = getString("c1").toUtf8();
		QByteArray c2 = getString("c2").toUtf8();
		if (c2.isEmpty()) c2 = c1;

		//create hash with information from 'in2'
		QHash<QByteArray, QByteArray> in2_data;
		TSVFileStream in2_stream(getInfile("in2"));
		int c2_index = in2_stream.colIndex(c2, true);
		QList<int> anno_indices;
		foreach(QByteArray col, anno)
		{
			anno_indices << in2_stream.colIndex(col, true);
		}
		QByteArrayList tmp;
		while(!in2_stream.atEnd())
		{
			QByteArrayList parts = in2_stream.readLine();
			if (parts.isEmpty()) continue;

			tmp.clear();
			foreach(int col_index, anno_indices)
			{
				tmp << parts[col_index];
			}
			QByteArray key = parts[c2_index];
			QByteArray value = tmp.join('\t');
			if (in2_data.contains(key) && in2_data[key]!=value) THROW(FileParseException, "Key '" + key + "' found several times in 'in2' and data in 'anno' columns differs!");
			in2_data[key] = value;
		}

		//write comments and header
		TSVFileStream in1_stream(getInfile("in1"));
		int c1_index = in1_stream.colIndex(c1, true);
		QSharedPointer<QFile> out_stream = Helper::openFileForWriting(getOutfile("out"), true);
		for(const QByteArray& comment: in1_stream.comments())
		{
			out_stream->write(comment + "\n");
		}
		out_stream->write("#" + in1_stream.header().join('\t') + "\t" + anno.join('\t') + "\n");

		//write content lines
		while(!in1_stream.atEnd())
		{
			QByteArrayList parts = in1_stream.readLine();
			if (parts.isEmpty()) continue;

			QByteArray key = parts[c1_index];
			out_stream->write(parts.join('\t') + "\t" + (in2_data.contains(key) ? in2_data[key] : missing_value) + "\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

