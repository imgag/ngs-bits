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
		setDescription("Extracts the phased genotype into seperate column.");
		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);
		addFlag("include_unphased", "Also annotate genotype of unphased SVs.");

		changeLog(2023, 9, 22, "Initial commit.");
		changeLog(2023, 10, 4, "Added parameter to also annotate unphased genotype.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");

		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		// create output buffer and copy comments and header
		QByteArrayList output_buffer;
		output_buffer.append(bedpe_file.headers());
		QByteArrayList updated_header;

		//check if multisample
		SampleHeaderInfo sample_header_info = bedpe_file.sampleHeaderInfo();
		bool is_multisample = (sample_header_info.size() > 1);
		QList<int> i_annotations;

		if (is_multisample)
		{
			// get header
			updated_header = bedpe_file.annotationHeaders();

			foreach (const SampleInfo& sample_info, sample_header_info)
			{
				// check if annotation already exisits:
				i_annotations.append(bedpe_file.annotationIndexByName(sample_info.name.toUtf8() + "_GENOTYPE", false));
				// modify header if genotype columns not already present
				if (i_annotations.last() < 0) updated_header.append(sample_info.name.toUtf8() + "_GENOTYPE");
			}
		}
		else //single sample
		{
			// check if annotation already exisits:
			i_annotations.append(bedpe_file.annotationIndexByName("GENOTYPE", false));
			// get header
			updated_header = bedpe_file.annotationHeaders();
			// modify header if genotype columns not already present
			if (i_annotations[0] < 0) updated_header.append("GENOTYPE");
		}

		// copy header
		output_buffer << "#CHROM_A\tSTART_A\tEND_A\tCHROM_B\tSTART_B\tEND_B\t" + updated_header.join("\t");

		for(int i=0; i<bedpe_file.count(); ++i)
		{
			BedpeLine line = bedpe_file[i];

			//add phasing to output
			QList<QByteArray> annotations = line.annotations();

			for (int s = 0; s < i_annotations.size(); ++s)
			{
				// get genotype/phasing
				int idx_format_value = -1;
				if (is_multisample) idx_format_value = sample_header_info.at(s).column_index;
				QByteArray genotype = getFormatValue("GT", line, bedpe_file.annotationHeaders(), idx_format_value);
				QByteArray phasing_block = getFormatValue("PS", line, bedpe_file.annotationHeaders(), idx_format_value, false);

				QByteArray phasing_entry;
				if (genotype.contains("|")) phasing_entry = genotype;
				else if(getFlag("include_unphased")) phasing_entry = genotype;
				if (!phasing_block.isEmpty() && phasing_block.trimmed() != ".") phasing_entry += " (" + phasing_block + ")";

				if (i_annotations[s] < 0)
				{
					annotations.append(phasing_entry);
				}
				else
				{
					annotations[i_annotations[s]] = phasing_entry;
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

private:
	QByteArray getFormatValue(QByteArray key, const BedpeLine& line, const QList<QByteArray>& annotation_headers, int idx_format_value = -1, bool error_on_missing_key = true)
	{
		int format_idx = annotation_headers.indexOf("FORMAT");
		if (format_idx < 0 ) THROW(ArgumentException, "No FORMAT column found!");
		int key_idx = line.annotations().at(format_idx).split(':').indexOf(key);
		if (key_idx < 0 )
		{
			if (!error_on_missing_key) return QByteArray();
			THROW(ArgumentException, "Key '" + key + "' not found in FORMAT column!");
		}
		if (idx_format_value < 0) idx_format_value = format_idx + 1;

		return line.annotations().at(idx_format_value).split(':').at(key_idx);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
