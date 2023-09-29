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
		setDescription("Extract the phased genotype into seperate column.");
		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);

		changeLog(2023, 9, 22, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");

		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		//check if multisample
		if (bedpe_file.sampleHeaderInfo().size() > 1) THROW(ArgumentException, "Multisamples are not supported!");

		// check if annotation already exisits:
		int i_annotation = bedpe_file.annotationIndexByName("genotype_phased", false);

		// create output buffer and copy comments and header
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_file.headers());
		// get header
		QByteArrayList updated_header = bedpe_file.annotationHeaders();
		// modify header if gene columns not already present
		if (i_annotation < 0) updated_header.prepend("genotype_phased");
		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + updated_header.join("\t");

		for(int i=0; i<bedpe_file.count(); ++i)
		{
			BedpeLine line = bedpe_file[i];

			// get genotype/phasing
			QByteArray phased_genotype = getFormatValue("GT", line, bedpe_file.annotationHeaders());
			QByteArray phasing_block = getFormatValue("GT", line, bedpe_file.annotationHeaders());

			QByteArray phasing_entry;
			if (phased_genotype.contains("|")) phasing_entry = phased_genotype + " (" + phasing_block + ")";

			//add phasing to output
			QList<QByteArray> annotations = line.annotations();
			if (i_annotation > -1)
			{
				annotations[i_annotation] = phasing_entry;
			}
			else
			{
				annotations.prepend(phasing_entry);
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

private:
	QByteArray getFormatValue(QByteArray key, const BedpeLine& line, const QList<QByteArray>& annotation_headers)
	{
		int format_idx = annotation_headers.indexOf("FORMAT");
		if (format_idx < 0 ) THROW(ArgumentException, "No FORMAT column found!");
		int key_idx = line.annotations().at(format_idx).split(':').indexOf(key);
		if (key_idx < 0 ) THROW(ArgumentException, "Key '" + key + "' not found in FORMAT column!");

		return line.annotations().at(format_idx + 1).split(':').at(key_idx);
	}
};
