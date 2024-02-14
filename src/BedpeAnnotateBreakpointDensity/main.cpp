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
		addInfile("density", "IGV density file containing break point density.", false);
		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);
		addInfile("density_sys", "Optional IGV density file containing break point density for a specific processing system.", true);


		changeLog(2022, 2, 23, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString density_file_path = getInfile("density");
		QString sys_density_file_path = getInfile("density_sys");
		bool optional_sys_file = !sys_density_file_path.isEmpty();
		QString out = getOutfile("out");
		int idx_density_column = 1;

		//load annotation database
		BedFile density_file;
		density_file.load(density_file_path);
		if (!density_file.isSorted()) density_file.sort();
		ChromosomalIndex<BedFile> anno_index(density_file);

		//load processing system specific annotation file
		BedFile sys_density_file;
		if (optional_sys_file)
		{
			sys_density_file.load(sys_density_file_path);
			if (!sys_density_file.isSorted()) sys_density_file.sort();
		}
		ChromosomalIndex<BedFile> sys_anno_index(sys_density_file);

		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		// check if annotation already exisits:
		int i_annotation = bedpe_file.annotationIndexByName("NGSD_SV_BREAKPOINT_DENSITY", false);
		int i_annotation_sys;
		if (optional_sys_file) i_annotation_sys = bedpe_file.annotationIndexByName("NGSD_SV_BREAKPOINT_DENSITY_SYS", false);


		// create output buffer and copy comments and header
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_file.headers());
		// get header
		QByteArrayList header = bedpe_file.annotationHeaders();
		// modify header if gene columns not already present
		if (i_annotation < 0) header.append("NGSD_SV_BREAKPOINT_DENSITY");
		if (optional_sys_file && i_annotation_sys < 0) header.append("NGSD_SV_BREAKPOINT_DENSITY_SYS");
		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + header.join("\t");


		for(int i=0; i<bedpe_file.count(); ++i)
		{
			BedpeLine line = bedpe_file[i];

			QByteArrayList density_annotation;
			QByteArrayList density_annotation_sys;


			// determine density for first break point
			QVector<int> density_bp1;
			QVector<int> indices = anno_index.matchingIndices(line.chr1(), line.start1() + 1, line.end1() + 1);
			//get all density values
			foreach(int index, indices)
			{
				density_bp1 << Helper::toInt(density_file[index].annotations()[idx_density_column], "SV break point density (BP1)", QByteArray::number(i));
			}
			density_annotation << QByteArray::number((*std::max_element(density_bp1.begin(), density_bp1.end())));

			//determine density for second break point
			//(insertions only have 1 breakpoint)
			if (line.type() != StructuralVariantType::INS)
			{
				QVector<int> density_bp2;
				indices = anno_index.matchingIndices(line.chr2(), line.start2() + 1, line.end2() + 1);
				//get all density values
				foreach(int index, indices)
				{
					density_bp2 << Helper::toInt(density_file[index].annotations()[idx_density_column], "SV break point density (BP2)", QByteArray::number(i));
				}
				density_annotation << QByteArray::number((*std::max_element(density_bp2.begin(), density_bp2.end())));
			}

			if (optional_sys_file)
			{
				// determine density for first break point
				QVector<int> density_bp1;
				QVector<int> indices = sys_anno_index.matchingIndices(line.chr1(), line.start1() + 1, line.end1() + 1);
				//get all density values
				foreach(int index, indices)
				{
					density_bp1 << Helper::toInt(sys_density_file[index].annotations()[idx_density_column], "SV break point density (BP1, sys_specific)", QByteArray::number(i));
				}
				density_annotation_sys << QByteArray::number((*std::max_element(density_bp1.begin(), density_bp1.end())));

				//determine density for second break point
				//(insertions only have 1 breakpoint)
				if (line.type() != StructuralVariantType::INS)
				{
					QVector<int> density_bp2;
					indices = sys_anno_index.matchingIndices(line.chr2(), line.start2() + 1, line.end2() + 1);
					//get all density values
					foreach(int index, indices)
					{
						density_bp2 << Helper::toInt(sys_density_file[index].annotations()[idx_density_column], "SV break point density (BP2, sys_specific)", QByteArray::number(i));
					}
					density_annotation_sys << QByteArray::number((*std::max_element(density_bp2.begin(), density_bp2.end())));
				}
			}

			//add annotation
			QList<QByteArray> annotations = line.annotations();
			if (i_annotation > -1)
			{
				annotations[i_annotation] = density_annotation.join(" / ");
			}
			else
			{
				annotations.append(density_annotation.join(" / "));
			}
			if (optional_sys_file)
			{
				if (i_annotation_sys > -1)
				{
					annotations[i_annotation_sys] = density_annotation_sys.join(" / ");
				}
				else
				{
					annotations.append(density_annotation_sys.join(" / "));
				}

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
