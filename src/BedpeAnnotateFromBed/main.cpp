#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "VcfFile.h"
#include "BedpeFile.h"
#include "Helper.h"
#include <QTextStream>
#include <QFileInfo>
#include <QElapsedTimer>

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
		setDescription("Annotates a BEDPE file with information from a BED file.");
		addInfile("bed", "BED file that is used as annotation source.", false);
		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);
		addInt("col", "Annotation source column (default: 4).", true, 4);
		addString("col_name", "Name of the annotated column", true, "ANNOTATION");
		addFlag("no_duplicates", "Remove duplicate annotations if several intervals from 'in2' overlap.");
		addFlag("url_decode", "Decode URL encoded characters.");
		addFlag("replace_underscore", "Replaces underscores with spaces in the annotation column.");

		changeLog(2020, 1, 27, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString bed = getInfile("bed");
		QString out = getOutfile("out");
		int i_col = getInt("col") - 4; //4'th column is the first annotation column
		QByteArray col_name = getString("col_name").toUtf8();
		bool no_duplicates = getFlag("no_duplicates");
		bool url_decode = getFlag("url_decode");
		bool replace_underscore = getFlag("replace_underscore");


		//load annotation database
		BedFile anno_file;
		anno_file.load(bed);
		if (!anno_file.isSorted()) anno_file.sort();
		ChromosomalIndex<BedFile> anno_index(anno_file);



		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		// check if annotation already exisits:
		int i_annotation = bedpe_file.annotationIndexByName(col_name, false);


		// create output buffer and copy comments and header
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_file.headers());
		// get header
		QByteArrayList header = bedpe_file.annotationHeaders();
		// modify header if gene columns not already present
		if (i_annotation < 0) header.append(col_name);
		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t");


		for(int i=0; i<bedpe_file.count(); ++i)
		{
			BedpeLine line = bedpe_file[i];
			BedFile affected_region = line.affectedRegion();

			//determine annotations
			QByteArrayList additional_annotations;

			for(int j=0; j<affected_region.count(); ++j)
			{
				BedLine& region = affected_region[j];
				QVector<int> indices = anno_index.matchingIndices(region.chr(), region.start(), region.end());
				foreach(int index, indices)
				{
					const BedLine& match = anno_file[index];
					bool anno_exists = match.annotations().count()>i_col;
					if (anno_exists)
					{
						additional_annotations << match.annotations()[i_col];
					}
				}
			}

			// format additional annotation
			if (no_duplicates)
			{
				std::sort(additional_annotations.begin(), additional_annotations.end());
				additional_annotations.erase(std::unique(additional_annotations.begin(), additional_annotations.end()), additional_annotations.end());
			}

			QByteArray additional_annotation_string = additional_annotations.join(";");

			if (url_decode)
			{
				additional_annotation_string = VcfFile::decodeInfoValue(additional_annotation_string).toUtf8();
			}

			if (replace_underscore)
			{
				additional_annotation_string = additional_annotation_string.replace("_", " ");
			}

			QList<QByteArray> annotations = line.annotations();
			if (i_annotation > -1)
			{
				annotations[i_annotation] = additional_annotation_string;
			}
			else
			{
				annotations.append(additional_annotation_string);
			}
			line.setAnnotations(annotations);

			//add annotated line to buffer
			output_buffer << line.toTsv();
		}


		// open output file and write annotated SVs to file
		QSharedPointer<QFile> cnv_output_file = Helper::openFileForWriting(out, true);
		QTextStream output_stream(cnv_output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
		}
		output_stream.flush();
		cnv_output_file->close();
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
