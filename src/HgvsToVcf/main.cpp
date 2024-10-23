#include "ToolBase.h"
#include "Helper.h"
#include "VcfFile.h"
#include "Transcript.h"
#include "NGSD.h"
#include <QDate>

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
		setDescription("Transforms a TSV file with transcript ID and HGVS.c change into a VCF file.");
		QStringList desc_ext;
		desc_ext << "Transforms a given TSV file with the transcript ID (e.g. ENST00000366955) in the first column and the HGVS.c change (e.g. c.8802A>G) in the second column into a VCF file.";
		desc_ext << "Any further columns of the input TSV file are added as info entries to the output VCF. The TSV column header is used as name for the info entries.";
		desc_ext << "Ensembl, CCDS and RefSeq transcript IDs can be given, the conversion is always based on the Ensembl transcripts. CCDS and RefSeq transcripts will be matched to an Ensembl transcript, if an identical one exists.";
		desc_ext << "When an input line can't be transformed into a VCF line a warning is printed to the console.";
		desc_ext << "";
		desc_ext << "Attention: This tool is experimental. Please report any errors!";
		setExtendedDescription(desc_ext);
		addOutfile("out", "Output VCF file.", false);
		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addString("input_info_field", "The input transcript ID and HGVS.c change are added to the VCF output using this INFO field name.", true, "HGVSc");
		addFlag("test", "Uses the test database instead of on the production database.");
		QStringList builds;
		builds << "hg19" << "hg38";
		addEnum("build", "Genome build", true, builds, "hg38");
		addInt("max_seq", "If set, skips variants with ref/alt sequence longer than this cutoff.", true, -1);

		changeLog(2022,  7, 25, "Added parameter 'max_seq'.");
		changeLog(2022,  5, 12, "Initial version");
	}

	void writeVcfHeaders(QSharedPointer<QFile>& outstream, const QStringList& tsv_headers, QString reference)
	{
		QStringList headers;
		headers << "##fileformat=VCFv4.2";
		headers << "##fileDate=" + QDate::currentDate().toString("yyyyMMdd");
		headers << "##source=ngs-bits:HgvsToVcf";
		headers << "##reference=" + reference;


		headers << "##INFO=<ID=" + getString("input_info_field") + ",Number=1,Type=String,Description=\"Input transcript name and HGVS.c change.\">";
		if (tsv_headers.count() > 2)
		{
			QString in = getInfile("in");
			if (in=="") in = "STDIN";

			for (int i=2; i<tsv_headers.count(); i++)
			{

				headers << "##INFO=<ID=" + tsv_headers[i] + ",Number=.,Type=String,Description=Column of HGVS TSV file: " + in + ">";
			}
		}

		headers << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO";

		foreach (const QString& line, headers)
		{
			outstream->write(line.toUtf8() + "\n");
		}
	}

	bool parseLine(const QString& line, NGSD& db, QSharedPointer<QFile>& outstream, const QStringList& tsv_headers, FastaFileIndex& ref_index, const QMap<QByteArray, QByteArrayList>& transcript_matches, int max_seq)
	{
		static QMap<QString, Transcript> name2transcript;

		QStringList parts = line.split("\t");
		if (parts.count()!=tsv_headers.count()) THROW(FileParseException, "Input TSV content line has " + QString::number(parts.count()) + " fields, but " + QString::number(tsv_headers.count()) + " are expected from header: " + line);

		//convert HGVS.c to VCF
		QByteArray transcript_name = parts[0].toUtf8();
		QByteArray hgvs_c = parts[1].toUtf8();

		try
		{
			Transcript transcript;
			//remove version number from transcript (if present)
			if (transcript_name.contains(".")) transcript_name = transcript_name.left(transcript_name.indexOf('.'));

			if (name2transcript.contains(transcript_name))
			{
				transcript = name2transcript[transcript_name];

				if (! transcript.isValid())
				{
					THROW(ArgumentException, "Transcript " + transcript_name + " not found in NGSD");
				}
			}
			else
			{
				//get transcript from NGSD
				int trans_id = db.transcriptId(transcript_name, false);

				if (trans_id==-1) //not found > check if it is a CCDS/RefSeq transcript
				{
					foreach(const QByteArray& match, transcript_matches[transcript_name])
					{
						if (match.startsWith("ENST"))
						{
							int match_id = db.transcriptId(match, false);
							if (match_id != -1)
							{
								trans_id = match_id;
							}
						}
					}
				}

				if (trans_id==-1) //not found > abort
				{
					name2transcript[transcript_name] = Transcript();
					THROW(ArgumentException, "Transcript " + transcript_name + " not found in NGSD");
				}

				transcript = db.transcript(trans_id);
				name2transcript[transcript_name] = transcript;
			}

			//convert to variant
			Variant variant = transcript.hgvsToVariant(hgvs_c, ref_index);

			//check variant
			variant.checkValid(ref_index);

			//skip too long variants
			if (max_seq>0 && (variant.obs().size()>max_seq || variant.ref().size()>max_seq)) return false;

			//write base variant line
			VcfLine vcf_rep = variant.toVCF(ref_index);
			QByteArray outline = vcf_rep.chr().strNormalized(true) + "\t" + QByteArray::number(vcf_rep.start()) + "\t.\t" + vcf_rep.ref() + "\t" + vcf_rep.altString() + "\t.\t.\t";

			//write info fields
			QByteArrayList info_fields;
			info_fields.append(getString("input_info_field").toUtf8() + "=" + parts[0].toUtf8() + ":" + parts[1].toUtf8());
			for(int i=2; i< parts.length(); i++)
			{
				info_fields.append(tsv_headers[i].toUtf8()+"="+parts[i].toUtf8());
			}
			outline += info_fields.join(";");

			outstream->write(outline + "\n");
			return true;
		}
		catch (Exception& e)
		{
			QTextStream out(stderr);
			out << "Warning: " + transcript_name + ":" + hgvs_c + " skipped: couldn't transform it to valid VCF: " + e.message() + "\n";
			return false;
		}
	}


	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString ref_file = getInfile("ref");
		int max_seq = getInt("max_seq");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex ref_index(ref_file);

		QSharedPointer<QFile> instream = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, false);

		if (getEnum("build") == "hg19")
		{
			QTextStream out(stderr);
			out << "Warning: When using the hg19 build, it is neccessary to also use a NGSD instance containing hg19 data and a hg19 reference genome.\n";
		}

		const QMap<QByteArray, QByteArrayList>& transcript_matches = NGSHelper::transcriptMatches(stringToBuild(getEnum("build")));

		QStringList tsv_headers = {"", ""}; //fallback in case there is no header
		NGSD db(getFlag("test"));
		bool header_written = false;
		while (!instream->atEnd())
		{
			QString line = instream->readLine();
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			//ignore empty lines and comments at the start of the file
			if (line.trimmed().isEmpty()) continue;
			if (line.startsWith("##")) continue;

			//handle headers
			if (line.startsWith('#'))
			{
				tsv_headers = line.split("\t");
				if (tsv_headers.count()<2) THROW(FileParseException, "Input TSV header line has less than two parts: " + line);
				for(int i=2; i<tsv_headers.count(); ++i)
				{
					QString header = tsv_headers[i];
					if (header.contains(';') || header.contains('=')) THROW(FileParseException, "TSV header is no valid VCF info key: " + header);
				}
				continue;
			}

			//write VCF header before first content line
			if (!header_written)
			{
				writeVcfHeaders(outstream, tsv_headers, ref_file);
				header_written = true;
			}

			//process content line
			parseLine(line, db, outstream, tsv_headers, ref_index, transcript_matches, max_seq);
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

