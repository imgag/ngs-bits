#include "ToolBase.h"
#include "Helper.h"
#include "VcfFile.h"
#include "Settings.h"

class ConcreteTool: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Prunes source variant annotations, if identical to shown variant.");

		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addFlag("verbose", "Outputs number of source variant annotations deleted due to indel leftalignment");

		changeLog(2025, 12,  8, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QTextStream out_stream(stdout);
		bool verbose = getFlag("verbose");
		int count = 0;

		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if (in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);

		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//write header unchanged
			if (line.startsWith("#"))
			{
				out_p->write(line);
				continue;
			}

			//split line and extract variant infos
			QByteArrayList parts = line.trimmed().split('\t');
			QByteArray variant = parts[VcfFile::CHROM] + "&" + parts[VcfFile::POS] + "&" + parts[VcfFile::REF] + "&" + parts[VcfFile::ALT];
			QByteArray info = parts[VcfFile::INFO];

			//get annotated source variant and remove if identical with variant
			if (info.contains("SOURCE_VAR"))
			{
				QByteArrayList info_parts = info.trimmed().split(';');
				QByteArrayList new_infos;

				for (const QByteArray &p : info_parts)
				{
					if (p.startsWith("SOURCE_VAR"))
					{
						QByteArray source_variant;
						source_variant = p.split('=')[1];
						if (source_variant == variant) continue;
						else
						{
							//TODO: check if new variant came from left normalize
							Variant var_source;
							var_source.setChr(source_variant.split('&')[0]);
							var_source.setStart(source_variant.split('&')[1].toInt());
							var_source.setEnd(source_variant.split('&')[1].toInt());
							var_source.setRef(source_variant.split('&')[2]);
							var_source.setObs(source_variant.split('&')[3]);

							Variant var;
							var.setChr(parts[VcfFile::CHROM]);
							var.setStart(parts[VcfFile::POS].toInt());
							var.setEnd(parts[VcfFile::POS].toInt());
							var.setRef(parts[VcfFile::REF]);
							var.setObs(parts[VcfFile::ALT]);

							if (!var_source.isSNV())
							{
								var_source.normalize();
								var_source.leftAlign(reference);

								if (var == var_source)
								{
									++count;
									continue;
								}
							}
						}
					}

					new_infos.append(p);
				}

				parts[VcfFile::INFO] = new_infos.join(";");
			}

			//output line
			out_p->write(parts.join('\t').append('\n'));
		}

		if (verbose) out_stream << "Source variant annotations deleted due to indel left align: " << count << QT_ENDL;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
