#include "LovdUploadFile.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSHelper.h"

QByteArray LovdUploadFile::create(QString sample, QString gender, QString gene, const Phenotype& pheno, const VariantList& vl, const Variant& variant)
{
	QByteArray output;
	QTextStream stream(&output);

	//create header part
	QString lab = getSettings("lovd_lab");
	QString user_name = getSettings("lovd_user_name");
	QString user_email = getSettings("lovd_user_email");
	QString user_id = getSettings("lovd_user_id");
	QString user_auth_token = getSettings("lovd_user_auth_token");

	stream << "{\n";
	stream << "    \"lsdb\": {\n";
	stream << "        \"@id\": \"53786324d4c6cf1d33a3e594a92591aa\",\n";
	stream << "        \"@uri\": \"http://databases.lovd.nl/shared/\",\n";
	stream << "        \"source\": {\n";
	stream << "            \"name\": \"" << lab << "\",\n";
	stream << "            \"contact\": {\n";
	stream << "                \"name\": \"" << user_name << "\",\n";
	stream << "                \"email\": \"" << user_email << "\",\n";
	stream << "                \"db_xref\": [\n";
	stream << "                    {\n";
	stream << "                        \"@source\": \"lovd\",\n";
	stream << "                        \"@accession\": \"" << user_id << "\"\n";
	stream << "                    },\n";
	stream << "                    {\n";
	stream << "                        \"@source\": \"lovd_auth_token\",\n";
	stream << "                        \"@accession\": \"" << user_auth_token << "\"\n";
	stream << "                    }\n";
	stream << "                ]\n";
	stream << "            }\n";
	stream << "        },\n";

	//create patient part
	stream << "        \"individual\": [\n";
	stream << "            {\n";
	stream << "                \"@id\": \"" << sample << "\",\n";
	stream << "                \"gender\": {\n";
	stream << "                    \"@code\": \"" << convertGender(gender) <<"\"\n";
	stream << "                },\n";
	stream << "                \"phenotype\": [\n"; //TODO support several phenotypes?
	stream << "                    {\n";
	stream << "                        \"@term\": \"" << pheno.name() << "\",\n";
	stream << "                        \"@source\": \"HPO\",\n";
	stream << "                        \"@accession\": \"" << pheno.accession().mid(3) << "\"\n";
	stream << "                    }\n";
	stream << "                ],\n";

	//general variant info
	QString genotype = getAnnotation(vl, variant, "genotype");
	QString classification = getAnnotation(vl, variant, "classification");
	stream << "                \"variant\": [\n";
	stream << "                    {\n";
	stream << "                        \"@copy_count\": \"" << convertGenotype(genotype) << "\",\n";
	stream << "                        \"@type\": \"DNA\",\n";
	stream << "                        \"ref_seq\": {\n";
	stream << "                            \"@source\": \"genbank\",\n";
	stream << "                            \"@accession\": \"" << chromosomeToAccession(variant.chr()) << "\"\n"; //official identifier for hg19
	stream << "                        },\n";
	stream << "                        \"name\": {\n";
	stream << "                            \"@scheme\": \"HGVS\",\n";
	FastaFileIndex idx(Settings::string("reference_genome"));
	stream << "                            \"#text\": \"" << variant.toHGVS(idx) << "\"\n";
	stream << "                        },\n";
	stream << "                        \"pathogenicity\": {\n";
	stream << "                            \"@scope\": \"individual\",\n";
	stream << "                            \"@term\": \"" << convertClassification(classification) << "\"\n";
	stream << "                        },\n";
	stream << "                        \"variant_detection\": [\n";
	stream << "                            {\n";
	stream << "                                \"@template\": \"DNA\",\n";
	stream << "                                \"@technique\": \"SEQ\"\n";
	stream << "                            }\n";
	stream << "                        ],\n";
	stream << "                        \"seq_changes\": {\n";
	stream << "                            \"variant\": [\n";

	QStringList transcripts = getAnnotation(vl, variant, "coding_and_splicing").split(',');
	foreach(QString transcript, transcripts)
	{
		if (!transcript.startsWith(gene + ":")) continue;
		QStringList parts = transcript.split(":");
		if (parts.count()<7)
		{
			THROW(ProgrammingException, "SnpEff ANN transcript has less than 7 parts: " + transcript);
		}
		QString nm_number = parts[1];
		QString hgvs_c = parts[5];
		QString hgvs_r = "r.(?)";
		QString hgvs_p = parts[6];
		stream << "                                {\n";
		stream << "                                    \"@type\": \"cDNA\",\n";
		stream << "                                    \"gene\": {\n";
		stream << "                                        \"@source\": \"HGNC\",\n";
		stream << "                                        \"@accession\": \"" << gene << "\"\n";
		stream << "                                    },\n";
		stream << "                                    \"ref_seq\": {\n";
		stream << "                                        \"@source\": \"genbank\",\n";
		stream << "                                        \"@accession\": \"" << nm_number << "\"\n";
		stream << "                                    },\n";
		stream << "                                    \"name\": {\n";
		stream << "                                        \"@scheme\": \"HGVS\",\n";
		stream << "                                        \"#text\": \"" << hgvs_c << "\"\n";
		stream << "                                    },\n";
		stream << "                                    \"seq_changes\": {\n";
		stream << "                                        \"variant\": [\n";
		stream << "                                            {\n";
		stream << "                                                \"@type\": \"RNA\",\n";
		stream << "                                                \"name\": {\n";
		stream << "                                                    \"@scheme\": \"HGVS\",\n";
		stream << "                                                    \"#text\": \"" << hgvs_r << "\"\n";
		stream << "                                                }";
		if (hgvs_p=="")
		{
			stream << "\n";
		}
		else
		{
			stream << ",\n";
			stream << "                                                \"seq_changes\": {\n";
			stream << "                                                    \"variant\": [\n";
			stream << "                                                        {\n";
			stream << "                                                            \"@type\": \"AA\",\n";
			stream << "                                                            \"name\": {\n";
			stream << "                                                                \"@scheme\": \"HGVS\",\n";
			stream << "                                                                \"#text\": \"" << hgvs_p << "\"\n";
			stream << "                                                            }\n";
			stream << "                                                        }\n";
			stream << "                                                    ]\n";
			stream << "                                                }\n";
		}
		stream << "                                            }\n";
		stream << "                                        ]\n";
		stream << "                                    }\n";
		stream << "                                }";
		if (transcript!=transcripts.last())
		{
			stream << ",";
		}
		stream << "\n";
	}

	//close all brackets
	stream << "                            ]\n";
	stream << "                        }\n";
	stream << "                    }\n";
	stream << "                ]\n";
	stream << "            }\n";
	stream << "        ]\n";
	stream << "    }\n";
	stream << "}\n";
	stream << "\n";

	return output;
}

QString LovdUploadFile::getSettings(QString key)
{
	QString output = Settings::string(key).trimmed();
	if (output.isEmpty())
	{
		THROW(FileParseException, "Settings INI file does not contain key '" + key + "'!");
	}
	return output;
}

QString LovdUploadFile::getAnnotation(const VariantList& vl, const Variant& variant, QString key)
{
	return variant.annotations()[vl.annotationIndexByName(key)];
}

QString LovdUploadFile::convertClassification(QString classification)
{
	if (classification=="5")
	{
		return "Pathogenic";
	}
	if (classification=="4")
	{
		return "Probably Pathogenic";
	}
	if (classification=="3" || classification=="n/a" || classification=="")
	{
		return "Not Known";
	}
	if (classification=="2")
	{
		return "Probably Not Pathogenic";
	}
	if (classification=="1")
	{
		return "Non-pathogenic";
	}

	THROW(ProgrammingException, "Unknown classification '" + classification + "' in LovdUploadFile::create(...) method!");
}

QString LovdUploadFile::chromosomeToAccession(const Chromosome& chr)
{
	QByteArray chr_str = chr.strNormalized(false);
	if (chr_str=="1") return "NC_000001.10";
	else if (chr_str=="2") return "NC_000002.11";
	else if (chr_str=="3") return "NC_000003.11";
	else if (chr_str=="4") return "NC_000004.11";
	else if (chr_str=="5") return "NC_000005.9";
	else if (chr_str=="6") return "NC_000006.11";
	else if (chr_str=="7") return "NC_000007.13";
	else if (chr_str=="8") return "NC_000008.10";
	else if (chr_str=="9") return "NC_000009.11";
	else if (chr_str=="10") return "NC_000010.10";
	else if (chr_str=="11") return "NC_000011.9";
	else if (chr_str=="12") return "NC_000012.11";
	else if (chr_str=="13") return "NC_000013.10";
	else if (chr_str=="14") return "NC_000014.8";
	else if (chr_str=="15") return "NC_000015.9";
	else if (chr_str=="16") return "NC_000016.9";
	else if (chr_str=="17") return "NC_000017.10";
	else if (chr_str=="18") return "NC_000018.9";
	else if (chr_str=="19") return "NC_000019.9";
	else if (chr_str=="20") return "NC_000020.10";
	else if (chr_str=="21") return "NC_000021.8";
	else if (chr_str=="22") return "NC_000022.10";
	else if (chr_str=="X") return "NC_000023.10";
	else if (chr_str=="Y") return "NC_000024.9";
	else if (chr_str=="MT") return "NC_012920.1";

	THROW(ProgrammingException, "Unknown chromosome '" + chr_str + "' in LovdUploadFile::create(...) method!");
}

QString LovdUploadFile::convertGender(QString gender)
{
	if (gender=="n/a")
	{
		return "0";
	}
	if (gender=="male")
	{
		return "1";
	}
	if (gender=="female")
	{
		return "2";
	}
	THROW(ProgrammingException, "Unknown gender '" + gender + "' in LovdUploadFile::create(...) method!");
}

QString LovdUploadFile::convertGenotype(QString genotype)
{
	if (genotype=="het")
	{
		return "1";
	}
	if (genotype=="hom")
	{
		return "2";
	}

	THROW(ProgrammingException, "Unknown genotype '" + genotype + "' in LovdUploadFile::create(...) method!")
}
