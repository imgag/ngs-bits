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
		setDescription("Annotates a BEDPE file with breakpoint density.");
		addInfile("bed", "BED file containing break point density.", false);
		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);


		changeLog(2022, 2, 23, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString bed = getInfile("bed");
		QString out = getOutfile("out");

		//load annotation database
		BedFile anno_file;
		anno_file.load(bed);
		if (!anno_file.isSorted()) anno_file.sort();
		ChromosomalIndex<BedFile> anno_index(anno_file);

		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		// check if annotation already exisits:
		int i_annotation = bedpe_file.annotationIndexByName("NGSD_SV_BREAKPOINT_DENSITY", false);


		// create output buffer and copy comments and header
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_file.headers());
		// get header
		QByteArrayList header = bedpe_file.annotationHeaders();
		// modify header if gene columns not already present
		if (i_annotation < 0) header.append("NGSD_SV_BREAKPOINT_DENSITY");
		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t");


		for(int i=0; i<bedpe_file.count(); ++i)
		{
			BedpeLine line = bedpe_file[i];
			BedFile region;
			region.append(BedLine(line.chr1(), line.start1() + 1, line.end1() + 1));
			if (line.type() != StructuralVariantType::INS)
			{
				//insertions only have 1 breakpoint
				region.append(BedLine(line.chr2(), line.start2() + 1, line.end2() + 1));
			}

			//determine annotations
			QByteArrayList additional_annotations;

			for(int j=0; j<region.count(); ++j)
			{
				BedLine& line = region[j];
				QVector<int> indices = anno_index.matchingIndices(line.chr(), line.start(), line.end());
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
			if (max_value)
			{
				if (additional_annotations.size() > 0)
				{
					double max_value = Helper::toDouble(additional_annotations.at(0));
					for (int i = 1; i < additional_annotations.size(); ++i)
					{
						max_value = std::max(max_value, Helper::toDouble(additional_annotations.at(i)));
					}
					// format value
					additional_annotations = QByteArrayList() << QByteArray::number(max_value, 'f', (fmod(max_value, 1) == 0.0)? 0: 4);
				}
			}

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
