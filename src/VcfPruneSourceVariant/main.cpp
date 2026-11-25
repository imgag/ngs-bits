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
		setDescription("Removes annotated source variant, if unchanged during variant normalization.");
		setExtendedDescription(QStringList() << "Used to track the source variant of variants that are modified during variant normalization."
											 << "Usual tool sequence: VcfStoreSourceVariant, vcfallelicprimitives, VcfBreakMulti, VcfLeftNormalize, VcfPruneSroucesVariant");

		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);

		changeLog(2026, 11, 24, "Initial implementation.");
	}

	virtual void main()
	{
		//init
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

		//process input VCF
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
			QByteArrayList parts = line.split('\t');
			if (parts.count() <= VcfFile::MIN_COLS) THROW(FileParseException, "VCF line has too few columns: " + line);

			//check for multiallelic variants
			if (parts[VcfFile::ALT].contains(",")) THROW(ToolFailedException , "Unsupported multi-allelic variant found in line: " + line);

			//get annotated source variant and remove if identical with variant
			QByteArray info = parts[VcfFile::INFO];
			if (info.contains("SOURCE_VAR="))
			{
				QByteArray variant = parts[VcfFile::CHROM] + "_" + parts[VcfFile::POS] + "_" + parts[VcfFile::REF].toUpper() + "_" + parts[VcfFile::ALT].toUpper();

				QByteArrayList new_infos;
				foreach (QByteArray p, info.split(';'))
				{
					if (p.startsWith("SOURCE_VAR="))
					{
						//source variant and current variant match
						QByteArrayList source_variants = p.mid(11).split(',');
						bool match = false;
						foreach(const QByteArray& source_variant, source_variants)
						{
							if (source_variant == variant) match = true;
						}
						if (match) continue;

						//source variants are not left-normalized. Check if the variant is the same when left-normalizing the source variant
						match = false;
						foreach(const QByteArray& source_variant, source_variants)
						{
							QByteArrayList source_variant_parts = source_variant.split('_');
							if (source_variant_parts.count()!=4) THROW(ToolFailedException , "Source variant with part count other than 4 found in line: " + line);

							int pos = Helper::toInt(source_variant_parts[1], "VCF position");
							VcfLine var_source(source_variant_parts[0], pos, source_variant_parts[2].toUpper(), QList<Sequence>() << source_variant_parts[3].toUpper());
							var_source.leftNormalize(reference);

							source_variant_parts[1] = QByteArray::number(var_source.start());
							source_variant_parts[2] = var_source.ref().toUpper();
							source_variant_parts[3] = var_source.altString().toUpper();
							if (source_variant_parts.join('_') == variant) match=true;
						}
						if (match) continue;
					}

					new_infos.append(p);
				}

				if (new_infos.isEmpty()) new_infos.append(".");
				parts[VcfFile::INFO] = new_infos.join(";");
				out_p->write(parts.join('\t'));
			}
			else
			{
				out_p->write(line);
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
