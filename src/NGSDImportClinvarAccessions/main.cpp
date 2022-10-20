#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"

#include <ChainFileReader.h>


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
		setDescription("Imports accession IDs for published varaints");
		addInfileList("in", "Submission log files of the ClinVar XML upload containing the accession ID", false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		NGSD db(getFlag("test"));
		QStringList input_files = getInfileList("in");
		FastaFileIndex genome_reference_hg19(Settings::string("reference_genome_hg19"));

		QMap<QByteArray, QPair<QByteArray, QByteArray>> accession_ids;

		foreach (const QString& file_path, input_files)
		{
			//parse file
			QSharedPointer<QFile> file = Helper::openFileForReading(file_path);
			QTextStream stream(file.data());
			QByteArray submission_id = QFileInfo(file_path).fileName().split('_').at(0).toUtf8();
			int var_idx = -1;
			while (!stream.atEnd())
			{
				QString line = stream.readLine();
				//skip header/comments:
				if(line.startsWith('#'))
				{
					if (line.startsWith("#Your_variant_id\t"))
					{
						QStringList header_items = line.split('\t');

						if(header_items.contains("Your_variant_description"))
						{
							var_idx = header_items.indexOf("Your_variant_description");
						}
						else if(header_items.contains("Your_variant_description_chromosome_coordinates"))
						{
							var_idx = header_items.indexOf("Your_variant_description_chromosome_coordinates");
						}
						else
						{
							THROW(FileParseException, "Cannot find info column for variant coordinates! 1 " + submission_id)
						}
						qDebug() << var_idx;

					}
					continue;
				}
				if(var_idx < 0) THROW(FileParseException, "Cannot find info column for variant coordinates! " + submission_id);
				QStringList parts = line.split("\t");
				QByteArray accession_id = parts[4].toUtf8();
				if (accession_id.contains('.')) accession_id = accession_id.split('.').at(0); //remove version

				QStringList variant_parts = parts[var_idx].replace("<", "").replace("/>", "").split(" ");

				Chromosome chr = Chromosome("chr" + variant_parts[2].split("=")[1].replace("\"", "").trimmed());
				int pos = Helper::toInt(variant_parts[5].split("=")[1].replace("\"", "").trimmed());
				QByteArray ref = variant_parts[4].split("=")[1].replace("\"", "").trimmed().toUtf8();
				QByteArray obs = variant_parts[3].split("=")[1].replace("\"", "").trimmed().toUtf8();

				accession_ids.insert(chr.strNormalized(true) + ":" + QByteArray::number(pos) + " " + ref + ">" + obs, QPair<QByteArray, QByteArray>(submission_id, accession_id));
			}

			file->close();
		}

		qDebug() << "input files parsed";

		// get all published variants without accession
		QList<int> pub_var_ids = db.getValuesInt("SELECT id FROM variant_publication WHERE db='ClinVar' AND result IS NULL");
		int n_match_found = 0;
		int n_no_match = 0;
		QList<QPair<int, QByteArray>> to_update;

		foreach (int pub_var_id, pub_var_ids)
		{
			QString var_id = db.getValue("SELECT variant_id FROM variant_publication WHERE id=:0", false, QByteArray::number(pub_var_id)).toString();
			Variant var = db.variant(var_id);
			Variant var_hg19 = liftOverVariant(var, false);

			VariantVcfRepresentation var_hg19_vcf = var_hg19.toVCF(genome_reference_hg19);
//			qDebug() << var_hg19_vcf.chr.str() << var_hg19_vcf.pos << var_hg19_vcf.ref << var_hg19_vcf.alt;

			QByteArray vcf_string = var_hg19_vcf.chr.str() + ":" + QByteArray::number(var_hg19_vcf.pos) + " " + var_hg19_vcf.ref + ">" + var_hg19_vcf.alt;

			if(accession_ids.contains(vcf_string))
			{
				QByteArray submission_id = accession_ids.value(vcf_string).first;
				QByteArray accession_id = accession_ids.value(vcf_string).second;
//				qDebug() << var_id << vcf_string << submission_id << accession_id;

				to_update.append(QPair<int, QByteArray>(pub_var_id, accession_id));
				n_match_found++;

//				qDebug() << ("UPDATE variant_publication SET result='processed;" + accession_id + "' WHERE id=" + QByteArray::number(pub_var_id) + " AND variant_id=" + var_id);
				db.getQuery().exec("UPDATE variant_publication SET result='processed;" + accession_id + "' WHERE id=" + QByteArray::number(pub_var_id) + " AND variant_id=" + var_id);

				QString details = db.getValue("SELECT details FROM variant_publication WHERE id=:0", false, QByteArray::number(pub_var_id)).toString();
				details = "submission_id=" + submission_id + ";" + details;
				qDebug() << accession_id << details;
				qDebug() << ("UPDATE variant_publication SET details='" + details + "' WHERE id=" + QByteArray::number(pub_var_id) + " AND variant_id=" + var_id);
				db.getQuery().exec("UPDATE variant_publication SET details='" + details + "' WHERE id=" + QByteArray::number(pub_var_id) + " AND variant_id=" + var_id);

			}
			else
			{
				n_no_match++;
			}

		}
		qDebug() << "samples without accession id:" << pub_var_ids.size();
		qDebug() << "matches found: " << n_match_found;
		qDebug() << "no matches found: " << n_no_match;

		qDebug() << "finished";



	}

	BedLine liftOver(const Chromosome& chr, int start, int end, bool hg19_to_hg38)
	{
		//special handling of chrMT (they are the same for GRCh37 and GRCh38)
		if (chr.strNormalized(true)=="chrMT") return BedLine(chr, start, end);

		//cache chain file readers to speed up multiple calls
		static QHash<QString, QSharedPointer<ChainFileReader>> chain_reader_cache;
		QString chain = hg19_to_hg38 ? "hg19_hg38" : "hg38_hg19";
		if (!chain_reader_cache.contains(chain))
		{
			QString filepath = Settings::string("liftover_" + chain, true).trimmed();
			if (filepath.isEmpty()) THROW(ProgrammingException, "No chain file specified in settings.ini. Please inform the bioinformatics team!");
			chain_reader_cache.insert(chain, QSharedPointer<ChainFileReader>(new ChainFileReader(filepath, 0.05)));
		}

		//lift region
		BedLine region = chain_reader_cache[chain]->lift(chr, start, end);

		if (!region.isValid()) THROW(ArgumentException, "genomic coordinate lift-over failed: Lifted region is not a valid region");

		return region;
	}

	Variant liftOverVariant(const Variant& v, bool hg19_to_hg38)
	{
		//load genome indices (static to do it only once)
		static FastaFileIndex genome_index(Settings::string("reference_genome"));
		static FastaFileIndex genome_index_hg19(Settings::string("reference_genome_hg19"));

		//lift-over coordinates
		BedLine coords_new = liftOver(v.chr(), v.start(), v.end(), hg19_to_hg38);
		if (v.chr().isNonSpecial() && !coords_new.chr().isNonSpecial())
		{
			THROW(ArgumentException, "Chromosome changed to special chromosome: "+v.chr().strNormalized(true)+" > "+coords_new.chr().strNormalized(true));
		}

		//init new variant
		Variant v2;
		v2.setChr(coords_new.chr());
		v2.setStart(coords_new.start());
		v2.setEnd(coords_new.end());

		//check sequence context is the same
		Sequence context_old;
		Sequence context_new;
		int context_length = 10 + v.ref().length();
		if (hg19_to_hg38)
		{
			context_old = genome_index_hg19.seq(v.chr(), v.start()-5, context_length);
			context_new = genome_index.seq(v2.chr(), v2.start()-5, context_length);
		}
		else
		{
			context_old = genome_index.seq(v.chr(), v.start()-5, context_length);
			context_new = genome_index_hg19.seq(v2.chr(), v2.start()-5, context_length);
		}
		if (context_old==context_new)
		{
			v2.setRef(v.ref());
			v2.setObs(v.obs());
		}
		else //check if strand changed, e.g. in NIPA1, GDF2, ANKRD35, TPTE, ...
		{
			context_new.reverseComplement();
			if (context_old==context_new)
			{
				Sequence ref = v.ref();
				if (ref!="-") ref.reverseComplement();
				v2.setRef(ref);

				Sequence obs = v.obs();
				if (obs!="-") obs.reverseComplement();
				v2.setObs(obs);
			}
			else
			{
				context_new.reverseComplement();
				THROW(ArgumentException, "Sequence context of variant changed: "+context_old+" > "+context_new);
			}
		}

		return v2;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
