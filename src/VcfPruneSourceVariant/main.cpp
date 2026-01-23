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
		addInfile("in", "Input VCF file. If unset, reads from STDIN. Does not support multiallelic variants", true, true);
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

		if (verbose) out_stream << "Source variant annotations deleted due to indel left align: " << QT_ENDL;

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

			if (parts.count() <= VcfFile::FORMAT)
			{
				out_p->write(line);
				continue;
			}

			//check for multiallelic variants
			if (parts[VcfFile::ALT].contains(",")) THROW(ToolFailedException , "Found multiallelic variant at position '" + parts[VcfFile::POS] + "'. Multiallelic variants are not supported by this tool!");

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
						QByteArrayList kv = p.split('=');

						if (kv.count() != 2) { new_infos.append(p); continue; }
						source_variant = kv[1];

						if (source_variant == variant) continue;
						else
						{
							//Check if new variant came from left normalize
							QByteArrayList sv = source_variant.split('&');
							if (sv.count() != 4) { new_infos.append(p); continue; }

							Chromosome chr = sv[0];
							int pos = Helper::toInt(sv[1], "VCF position");
							Sequence ref = sv[2].toUpper();
							QByteArray alt = sv[3].toUpper();

							VcfLine var_source(chr, pos, ref, QList<Sequence>() << alt);

							if (!var_source.isSNV())
							{
								var_source.leftNormalize(reference);

								Chromosome chr = parts[VcfFile::CHROM];
								int pos = Helper::toInt(parts[VcfFile::POS], "VCF position");
								Sequence ref = parts[VcfFile::REF].toUpper();
								QByteArray alt = parts[VcfFile::ALT].toUpper();

								VcfLine var(chr, pos, ref, QList<Sequence>() << alt);

								if (var == var_source)
								{
									if (verbose) out_stream << source_variant << QT_ENDL;
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

		if (verbose) out_stream << "Number of source variant annotations removed due to indel left align: " << count << QT_ENDL;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
