#include "LovdUploadFile.h"
#include "Settings.h"
#include "Exceptions.h"

QString LovdUploadFile::create(QString sample, QString gender, Phenotype pheno, const VariantList& vl, int variant_index)
{
	QString output;
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
	if (gender=="n/a") gender = "0";
	else if (gender=="male") gender = "1";
	else if (gender=="female") gender = "2";
	else THROW(ProgrammingException, "Unknown gender '" + gender + "' in LovdUploadFile::create(...) method!");

	stream << "        \"individual\": [\n";
	stream << "            {\n";
	stream << "                \"@id\": \"" << sample << "\",\n";
	stream << "                \"gender\": {\n";
	stream << "                    \"@code\": \"" << gender <<"\"\n";
	stream << "                },\n";
	stream << "                \"phenotype\": [\n";
	stream << "                    {\n";
	stream << "                        \"@term\": \"" << pheno.name() << "\",\n";
	stream << "                        \"@source\": \"HPO\",\n";
	stream << "                        \"@accession\": \"" << pheno.accession() << "\"\n";
	stream << "                    }\n";
	stream << "                ],\n";

	//create variants part
	const Variant& v = vl[variant_index];
	QString genotype = v.annotations()[vl.annotationIndexByName("genotype")];
	QString copy_count;
	if (genotype=="het")
	{
		copy_count = "1";
	}
	else if (genotype=="hom")
	{
		copy_count = "2";
	}
	else
	{
		THROW(ProgrammingException, "Unknown genotype '" + genotype + "' in LovdUploadFile::create(...) method!")
	}
	stream << "                \"variant\": [\n";
	stream << "                    {\n";
	stream << "                        \"@copy_count\": \"" << copy_count << "\",\n";
	stream << "                        \"@type\": \"DNA\",\n";
	stream << "                        \"ref_seq\": {\n";
	stream << "                            \"@source\": \"genbank\",\n";
	stream << "                            \"@accession\": \"NC_000015.9\"\n";
	stream << "                        },\n";
	stream << "                        \"name\": {\n";
	stream << "                            \"@scheme\": \"HGVS\",\n";
	stream << "                            \"#text\": \"g.40702997G>A\"\n";
	stream << "                        },\n";
	stream << "                        \"pathogenicity\": {\n";
	stream << "                            \"@scope\": \"individual\",\n";
	stream << "                            \"@term\": \"Pathogenic\"\n";
	stream << "                        },\n";
	stream << "                        \"variant_detection\": [\n";
	stream << "                            {\n";
	stream << "                                \"@template\": \"DNA\",\n";
	stream << "                                \"@technique\": \"SEQ\"\n";
	stream << "                            },\n";
	stream << "                        ],\n";
	stream << "                        \"seq_changes\": {\n";
	stream << "                            \"variant\": [\n";
	stream << "                                {\n";
	stream << "                                    \"@type\": \"cDNA\",\n";
	stream << "                                    \"gene\": {\n";
	stream << "                                        \"@source\": \"HGNC\",\n";
	stream << "                                        \"@accession\": \"IVD\"\n";
	stream << "                                    },\n";
	stream << "                                    \"ref_seq\": {\n";
	stream << "                                        \"@source\": \"genbank\",\n";
	stream << "                                        \"@accession\": \"NM_002225.3\"\n";
	stream << "                                    },\n";
	stream << "                                    \"name\": {\n";
	stream << "                                        \"@scheme\": \"HGVS\",\n";
	stream << "                                        \"#text\": \"c.465+1G>A\"\n";
	stream << "                                    },\n";
	stream << "                                    \"seq_changes\": {\n";
	stream << "                                        \"variant\": [\n";
	stream << "                                            {\n";
	stream << "                                                \"@type\": \"RNA\",\n";
	stream << "                                                \"name\": {\n";
	stream << "                                                    \"@scheme\": \"HGVS\",\n";
	stream << "                                                    \"#text\": \"r.296_465del\"\n";
	stream << "                                                },\n";
	stream << "                                                \"seq_changes\": {\n";
	stream << "                                                    \"variant\": [\n";
	stream << "                                                        {\n";
	stream << "                                                            \"@type\": \"AA\",\n";
	stream << "                                                            \"name\": {\n";
	stream << "                                                                \"@scheme\": \"HGVS\",\n";
	stream << "                                                                \"#text\": \"p.Val99Alafs*5\"\n";
	stream << "                                                            }\n";
	stream << "                                                        }\n";
	stream << "                                                    ]\n";
	stream << "                                                }\n";
	stream << "                                            }\n";
	stream << "                                        ]\n";
	stream << "                                    }\n";
	stream << "                                }\n";
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
