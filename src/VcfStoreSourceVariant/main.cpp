#include "ToolBase.h"
#include "Helper.h"
#include "VcfFile.h"

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
		setDescription("Writes the variant to info field SOURCE_VAR for variants that are not SNPs.");
		setExtendedDescription(QStringList() << "Used to track the source variant of variants that are modified during variant normalization."
											 << "Usual tool sequence: VcfStoreSourceVariant, vcfallelicprimitives, VcfBreakMulti, VcfLeftNormalize, VcfPruneSroucesVariant");

		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF list. If unset, writes to STDOUT.", true, true);

		changeLog(2026, 11, 24, "Initial implementation.");
	}

	virtual void main()
	{
		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if (in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		bool info_header_set = false;
		while(!in_p->atEnd())
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//add SOURCE_VAR info header line if not present
			if (line.startsWith("#"))
			{
				if (line.startsWith("#CHROM") && !info_header_set)
				{
					out_p->write("##INFO=<ID=SOURCE_VAR,Number=A,Type=String,Description=\"Source variant before variant normalization.\">\n");
				}

				if (line.startsWith("##INFO=<ID=SOURCE_VAR,"))
				{
					info_header_set = true;
				}

				out_p->write(line);
				continue;
			}

			//process variant lines
			QByteArrayList parts = line.split('\t');
			QByteArray ref = parts[VcfFile::REF].toUpper();
			QByteArray alt = parts[VcfFile::ALT].toUpper();
			if (ref.length()==1 && alt.length()==1) //SNPs do not change during normalization > no need to annotate them
			{
				out_p->write(line);
			}
			else
			{
				//handle multiallelic sites
				QByteArrayList alts = alt.split(',');
				QByteArray anno = "SOURCE_VAR=" + parts[VcfFile::CHROM] + "_" + parts[VcfFile::POS] + "_" + ref + "_" + alts[0];
				for (int i = 1; i < alts.count(); i++)
				{
					anno.append("," + parts[VcfFile::CHROM] + "_" + parts[VcfFile::POS] + "_" + ref + "_" + alts[i]);
				}

				//remove previous annotation if present
				QByteArray info = parts[VcfFile::INFO];
				if (info.contains("SOURCE_VAR="))
				{
					QByteArrayList cleaned;
					for (const QByteArray& part : info.split(';'))
					{
						if (!part.startsWith("SOURCE_VAR=")) cleaned.append(part);
					}
					info = cleaned.join(";");
				}

				//add source variant annotation
				if (info==".") info.clear();
				if (!info.isEmpty()) info.append(";");
				info.append(anno);

				parts[VcfFile::INFO] = info;

				//output line
				out_p->write(parts.join('\t'));
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
