#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "BedpeFile.h"
#include <QTextStream>
#include <QFileInfo>


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
		setDescription("Extract a given INFO field key and annotates it as a separate column.");
		addString("info_fields", "Comma separate list of INFO keys (and column header names) which should be extracted: \"INFO_KEY1[:COLUMN_HEADER1],INFO_KEY2[:COLUMN_HEADER2],...\"", false);
		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);
		addString("info_column", "Header name of the INFO column.", true, "INFO_A");

		changeLog(2023, 10, 4, "Initial commit.");
		changeLog(2024, 1, 18, "Removed single sample restriction");

	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");

		//parse INFO->COLUMN mapping
		QMap<QByteArray,QByteArray> column_info_mapping;
		QByteArrayList columns;
		foreach (const QString& kv_pair, getString("info_fields").split(','))
		{
			QByteArray info, column;
			if (kv_pair.contains(':'))
			{
				//column will be renamed
				info = kv_pair.split(':').at(0).trimmed().toUtf8();
				column = kv_pair.split(':').at(1).trimmed().toUtf8();
			}
			else
			{
				//same name as info key
				info = kv_pair.trimmed().toUtf8();
				column = kv_pair.trimmed().toUtf8();
			}
			column_info_mapping.insert(column, info);
			columns.append(column);
		}

		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		// check if annotation already exisits:
		QMap<QByteArray,int> column_indices;
		foreach (const QByteArray& column, columns)
		{
			column_indices.insert(column, bedpe_file.annotationIndexByName(column, false));
		}

		// create output buffer and copy comments and header
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_file.headers());
		// get header
		QByteArrayList updated_header = bedpe_file.annotationHeaders();
		// modify header if columns not already present
		foreach (const QByteArray& column, columns)
		{
			if (column_indices.value(column) < 0) updated_header.append(column);
		}
		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + updated_header.join("\t");

		//get INFO column
		int info_idx = bedpe_file.annotationIndexByName(getString("info_column").toUtf8(), true);

		//parse BEDPE file
		for(int i=0; i<bedpe_file.count(); ++i)
		{
			BedpeLine line = bedpe_file[i];

			// parse info column
			QMap<QByteArray,QByteArray> info_values;
			QByteArray raw_info_entry = line.annotations().at(info_idx).trimmed();
			if (!(raw_info_entry.isEmpty() || (raw_info_entry == ".")))
			{
				QByteArrayList kv_pairs = raw_info_entry.split(';');
				foreach (const QByteArray& kv_pair, kv_pairs)
				{
					QByteArray key, value;
					if (kv_pair.contains('='))
					{
						key = kv_pair.split('=').at(0).trimmed();
						value = kv_pair.split('=').at(1).trimmed();
					}
					else
					{
						key = kv_pair.trimmed();
						value = "TRUE";
					}
					info_values.insert(key, value);
				}
			}

			//extract annotations
			QList<QByteArray> annotations = line.annotations();
			foreach (const QByteArray& column, columns)
			{
				const QByteArray& info_key = column_info_mapping.value(column);
				const QByteArray& info_value = info_values.value(info_key, "");

				int col_idx = column_indices.value(column);
				if (col_idx < 0) annotations.append(info_value);
				else annotations[col_idx] = info_value;
			}

			//apply additional annotations
			line.setAnnotations(annotations);

			//add annotated line to buffer
			output_buffer << line.toTsv();
		}

		// open output file and write annotated SVs to file
		QSharedPointer<QFile> bedpe_output_file = Helper::openFileForWriting(out, true);
		QTextStream output_stream(bedpe_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		bedpe_output_file->close();
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
