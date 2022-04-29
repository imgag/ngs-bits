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
		setDescription("Transforms a TSV file (col1: transcript ID; col 2: HGVS.c change ) into a VCF file. (Attention: This tool is experimental. Please report any errors!)");
		QStringList extDescription;
		extDescription << "Transforms a given TSV file with the transcript ID (e.g. ENST00000366955) in the first column and the HGVS.c change (e.g. c.8802A>G) in the second column into a vcf file.";
		extDescription << "Any further columns of the input TSV file are added as info entries to the output VCF. The TSV column header is used to name for the info entries.";
		extDescription << "Transcript IDs can be given in Ensembl, CCDS and RefSeq, but HGVS.c changes are transformed using Ensembl transcripts. CCDS and RefSeq transcripts will be matched to an Ensembl transcript, if an identical one exists.";
		extDescription << "When an input line can't be transformed into a VCF line a warning is printed to the console.";
		setExtendedDescription(extDescription);
		addOutfile("out", "Output VCF file.", false);
		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addString("hgvs_c", "The input transcript_ID:HGVS pair is added to the VCF output using this name.", true, "HGVSc");
		QStringList builds;
		builds << "hg19" << "hg38";
		addEnum("build", "Genome build", true, builds, "hg38");
	}


	const QMap<QByteArray, QByteArrayList>& transcriptMatches()
	{
		static QMap<QByteArray, QByteArrayList> output;
		static bool initialized = false;

		if (!initialized)
		{
			QStringList lines = Helper::loadTextFile(":/Resources/"+getEnum("build")+"_ensembl_transcript_matches.tsv", true, '#', true);
			foreach(const QString& line, lines)
			{
				QByteArrayList parts = line.toLatin1().split('\t');
				if (parts.count()>=2)
				{
					QByteArray enst = parts[0];
					QByteArray match = parts[1];
					output[enst] << match;
				}
			}
			initialized = true;
		}
		return output;
	}

	Variant hgvsToVariant(QString transcript_name, QString hgvs_c, NGSD& db, FastaFileIndex& ref_genome_idx)
	{

		static QMap<QString, Transcript> name2transcript;
		static QMap<QString, bool> bad_transcripts;
		Transcript transcript;

		// try to find it in buffer
		if (name2transcript.contains(transcript_name))
		{
			transcript = name2transcript[transcript_name];
		}
		else if (name2transcript.contains(transcript_name.left(transcript_name.indexOf('.'))))
		{
			transcript = name2transcript[transcript_name.left(transcript_name.indexOf('.'))];
		}
		else
		{
			//loock if base transcript was already searched without success:
			if (bad_transcripts.contains(transcript_name.left(transcript_name.indexOf('.'))))
			{
				QTextStream out(stdout);
				out << "ArgumentException:\tCannot determine Ensembl transcript for CCDS/RefSeq/Ensembl transcript identifier.\t" + transcript_name + ":" + hgvs_c + "\tTranscript not found in the database.\n";
				return Variant();
			}
			//Query NGSD to find transcript
			int trans_id = db.transcriptId(transcript_name, false);
			if (trans_id==-1) //not found > try to match CCDS/RefSeq toEnsembl
			{
				//remove version number (if present)
				if (transcript_name.contains("."))
				{
					transcript_name = transcript_name.left(transcript_name.indexOf('.'));
				}

				QString transcript_name2 = "";
				const QMap<QByteArray, QByteArrayList>& matches = transcriptMatches();
				for (auto it=matches.begin(); it!=matches.end(); ++it)
				{
					foreach(const QByteArray& trans, it.value())
					{
						if (trans==transcript_name)
						{
							transcript_name2 = it.key();
						}
					}
				}
				if (transcript_name2!="")
				{
					trans_id = db.transcriptId(transcript_name2, false);
				}
			}
			if (trans_id==-1)
			{
				bad_transcripts[transcript_name] = false;
				QTextStream out(stdout);
				out << "ArgumentException\tCannot determine Ensembl transcript for CCDS/RefSeq/Ensembl transcript identifier.\t" + transcript_name + ":" + hgvs_c + "\tTranscript not found in the database.\n";
				return Variant();
			}
			transcript = db.transcript(trans_id);
			// save new transcript in buffer
			name2transcript[transcript_name] = transcript;
		}
		Variant variant;

		try
		{
			variant = transcript.hgvsToVariant(hgvs_c, ref_genome_idx);
		}
		catch (ArgumentException e)
		{
			QTextStream out(stdout);
			out << "ArgumentException\tCouldn't transform given HGVSc into vcf with found transcript.\t" + transcript_name + ":" + hgvs_c + "\t" + e.message() + "\n";
			return Variant();
		}
		catch (ProgrammingException e)
		{
			QTextStream out(stdout);
			out << "ProgrammingException\tCouldn't transform given HGVSc into vcf with found transcript. \t" + transcript_name + ":" + hgvs_c + "\t" + e.message() + "\n";
			return Variant();
		}

		try
		{
			variant.checkValid();
			if (variant.ref()!="-") variant.checkReferenceSequence(ref_genome_idx);
			return variant;
		}
		catch (ArgumentException e)
		{

			QTextStream out(stdout);
			out << "ArgumentException\tInvalid resulting variant\t" + transcript_name + ":" + hgvs_c + "\t" + e.message() + "\n";
			return Variant();
		}
	}

	void writeVcfHeaders(QSharedPointer<QFile> outstream, QStringList tsv_headers, QString reference)
	{
		QStringList headers;
		headers << "##fileformat=VCFv4.2";
		headers << "##fileDate=" + QDate::currentDate().toString("yyyyMMdd");
		headers << "##source=ngs-bits:HgvsToVcf";
		headers << "##reference=" + reference;


		headers << "##INFO=<ID=" + getString("hgvs_c") + ",Number=.,Type=String,Description=transcript_name:HGVS_change.>";
		if (tsv_headers.count() > 2)
		{
			QString in = getInfile("in");
			if (in == "")
			{
				in = "STDIN";
			}
			for (int i=2; i<tsv_headers.count(); i++)
			{

				headers << "##INFO=<ID=" + tsv_headers[i] + ",Number=.,Type=String,Description=Column of HGVS TSV file: " + in + ">";
			}
		}

		headers << "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO";

		foreach (QString line, headers)
		{
			outstream->write(line.toLatin1() + "\n");
		}
	}

	void parseLine(QString line, NGSD& db, QSharedPointer<QFile> outstream, QStringList& tsv_headers, FastaFileIndex& ref_index)
	{
		QStringList parts = line.split("\t");

		Variant variant = hgvsToVariant(parts[0], parts[1], db, ref_index);
		if (! variant.isValid())
		{
			return;
		}
		VariantVcfRepresentation vcf_rep = variant.toVCF(ref_index);
		QByteArray outline = vcf_rep.chr.strNormalized(true) + "\t" + QByteArray::number(vcf_rep.pos) + "\t.\t" + vcf_rep.ref + "\t" + vcf_rep.alt + "\t.\t.\t";


		QByteArrayList info_fields;
		info_fields.append(getString("hgvs_c").toLatin1() + "=" + parts[0].toLatin1() + ":" + parts[1].toLatin1());

		if (parts.length() > 2)
		{

			for(int i=2; i< parts.length(); i++)
			{
				info_fields.append(tsv_headers[i].toLatin1()+"="+parts[i].toLatin1());
			}

		}
		outline += info_fields.join(";");
		outstream->write(outline + "\n");
	}


	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex ref_index(ref_file);

		QSharedPointer<QFile> instream = Helper::openFileForReading(in, true);
		QSharedPointer<QFile> outstream = Helper::openFileForWriting(out, false);

		QString line;
		QStringList tsv_headers;
		NGSD db;

		line = instream->readLine();
		line = line.trimmed();

		//ignore empty lines and comments at the start of the file
		while (line == "" || line.startsWith("##"))
		{
			line = instream->readLine();
			line = line.trimmed();
		}


		if ((! line.startsWith("#") && line.split("\t").count() != 2) || line.split("\t").count() < 2)
		{
			THROW(ArgumentException, "Malformed HGVS.tsv. Missing headers for a file with more than two columns or only one column found.");
		}

		if (line.startsWith("#"))
		{
			line = line.mid(1);
			tsv_headers = line.split("\t");
			line = instream->readLine();
			line = line.trimmed();
		}

		writeVcfHeaders(outstream, tsv_headers, ref_file);

		while (! instream->atEnd())
		{
			parseLine(line, db, outstream, tsv_headers, ref_index);
			//read next line
			line = instream->readLine();
			line = line.trimmed();
		}
		parseLine(line, db, outstream, tsv_headers, ref_index);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

