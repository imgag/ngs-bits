#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "VcfFile.h"
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
		setDescription("Annotates BED file regions with information from a second BED file.");
		addInfile("in2", "BED file that is used as annotation source.", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("col", "Annotation source column (if column number does not exist, 'yes' is used).", true, 4);
		addFlag("clear", "Clear all annotations present in the 'in' file.");
		addFlag("no_duplicates", "Remove duplicate annotations if several intervals from 'in2' overlap.");
		addFlag("overlap", "Annotate overlap with regions in 'in2'. The regular annotation is appended in brackets.");
		addFlag("url_decode", "Decode URL encoded characters");

		changeLog(2019,  7,  9, "Added parameters 'col', 'overlap' and 'no_duplicates'; Fixed 'clear' parameter.");
		changeLog(2017, 11, 28, "Added 'clear' flag.");
		changeLog(2017, 11, 03, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString in2 = getInfile("in2");
		QString out = getOutfile("out");
		int col = getInt("col") - 4; //4'th column is the first annotation column
		bool clear = getFlag("clear");
		bool no_duplicates = getFlag("no_duplicates");
		bool overlap = getFlag("overlap");
		bool url_decode = getFlag("url_decode");

		//load annoation database
		BedFile anno_file;
		anno_file.load(in2);
		anno_file.sort();
		ChromosomalIndex<BedFile> anno_index(anno_file);

		//process
		BedFile file;
		file.load(in);
		if (clear)
		{
			file.clearAnnotations();
		}
		for(int i=0; i<file.count(); ++i)
		{
			BedLine& line = file[i];

			//determine annotations and overlap
			QByteArrayList annos;
			BedFile overlap_regions;

			QVector<int> indices = anno_index.matchingIndices(line.chr(), line.start(), line.end());
			foreach(int index, indices)
			{
				const BedLine& match = anno_file[index];
				bool anno_exists = match.annotations().count()>col;
				if (anno_exists)
				{
					annos << match.annotations()[col];
				}
				else if (!overlap)
				{
					annos << "yes";
				}

				if (overlap)
				{
					overlap_regions.append(BedLine(line.chr(), std::max(line.start(),match.start()), std::min(line.end(),match.end())));
				}
			}
			if (no_duplicates)
			{
				std::sort(annos.begin(), annos.end());
				annos.erase(std::unique(annos.begin(), annos.end()), annos.end());
			}

			//construct annotation string
			QByteArray anno;
			if (overlap)
			{
				overlap_regions.merge();
				anno = QByteArray::number(1.0 * overlap_regions.baseCount() / line.length(), 'f', 3);
				if (annos.count()>0)
				{
					anno += " (" + annos.join(",") + ")";
				}
			}
			else
			{
				anno = annos.join(",");
			}

			if (url_decode)
			{
				anno = VcfFile::decodeInfoValue(anno).toUtf8();
			}
			line.annotations().append(anno);
		}

		//special handling of TSV files (handle header line)
		if (out.endsWith(".tsv"))
		{
			QVector<QByteArray> headers = file.headers();
			for(int i=0; i<headers.count(); ++i)
			{
				QByteArray& line = headers[i];
				if (line.startsWith("#") && !line.startsWith("##") && line.contains("\t"))
				{
					line += QByteArray("\t") + (overlap ? "overlap " : "") + QFileInfo(in2).baseName();
				}
			}
			file.setHeaders(headers);
		}

		//store
		file.store(out);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
