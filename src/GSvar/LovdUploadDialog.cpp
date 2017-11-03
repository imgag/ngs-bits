#include "LovdUploadDialog.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPrinter>
#include <QPrintDialog>

LovdUploadDialog::LovdUploadDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.upload_btn, SIGNAL(clicked(bool)), this, SLOT(upload()));

	connect(ui_.gene, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.nm_number, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_c, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_g, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.classification, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.phenos, SIGNAL(phenotypeSelectionChanged()), this, SLOT(checkGuiData()));
	connect(ui_.print_btn, SIGNAL(clicked(bool)), this, SLOT(printResults()));
	connect(ui_.comment_upload, SIGNAL(textChanged()), this, SLOT(updatePrintButton()));
}

void LovdUploadDialog::setData(const Variant& variant, LovdUploadData data)
{
	//fill in data
	data_ = data;
	dataToGui();

	//reference external webservice
	QString url = Settings::string("VariantInfoRefSeq");
	ui_.comment_var->setText("HGVS notation for RefSeq transcripts is not available in megSAP/GSvar. Thus it needs to be created by the external webservice <a href=\"" + url + "?variant_data=" + variant.toString(true).replace(" ", "\t") + "\">VariantInfoRefSeq</a>.\nPlease fill in the missing fields!");
}

void LovdUploadDialog::upload()
{
	//create JSON-formatted upload data
	guiToData();
	QByteArray upload_file = create(data_);

	//upload data
	static HttpHandler http_handler; //static to allow caching of credentials
	try
	{
		QString reply = http_handler.getHttpReply("https://databases.lovd.nl/shared/api/submissions", upload_file);
		ui_.comment_upload->setText(reply);

		//parse JSON result
		QJsonDocument json = QJsonDocument::fromJson(reply.toLatin1());
		QStringList messages;
		bool success = false;
		foreach(QJsonValue o, json.object()["messages"].toArray())
		{
			messages << "MESSAGE: " + o.toString();
			if (o.toString().startsWith("Data successfully scheduled for import."))
			{
				success = true;
			}
		}
		foreach(QJsonValue o, json.object()["errors"].toArray())
		{
			messages << "ERROR: " + o.toString();
		}
		foreach(QJsonValue o, json.object()["warnings"].toArray())
		{
			messages << "WARNING: " +o.toString();
		}

		//add entry to NGSD
		if (success)
		{
			QStringList details;
			details << "gene=" + data_.gene;
			details << "transcript=" + data_.nm_number;
			details << "hgvs_g=" + data_.hgvs_g;
			details << "hgvs_c=" + data_.hgvs_c;
			details << "hgvs_p=" + data_.hgvs_p;
			details << "genotype=" + data_.genotype;
			foreach(const Phenotype& pheno, data_.phenos)
			{

				details << "phenotype=" + pheno.accession() + " - " + pheno.name();
			}

			db_.addVariantPublication(data_.processed_sample, data_.variant, "LOVD", data_.classification, details.join(";"));

			//show result
			QStringList lines;
			lines << "DATA UPLOAD TO LOVD SUCCESSFUL";
			lines << "";
			lines << messages.join("\n");
			lines << "";
			lines << "sample: " + data_.processed_sample;
			lines << "variant: " + data_.variant.toString();
			lines << "classification: " + data_.classification;
			lines << "";
			lines << "user: " + Helper::userName();
			lines << "date: " + Helper::dateTime();
			lines << "";
			lines << details;

			ui_.comment_upload->setText(lines.join("\n").replace("=", ": "));
		}
		else
		{
			ui_.comment_upload->setText("DATA UPLOAD ERROR:\n" + messages.join("\n"));
		}

	}
	catch(Exception e)
	{
		ui_.comment_upload->setText("DATA UPLOAD FAILED:\n" + e.message());
	}
}

void LovdUploadDialog::dataToGui()
{
	//sample data
	ui_.processed_sample->setText(data_.processed_sample);
	ui_.gender->setText(data_.gender);
	ui_.genotype->setText(data_.genotype);

	//variant data
	ui_.chr->setText(data_.variant.chr().str());
	ui_.gene->setText(data_.gene);
	ui_.nm_number->setText(data_.nm_number);
	ui_.hgvs_g->setText(data_.hgvs_g);
	ui_.hgvs_c->setText(data_.hgvs_c);
	ui_.hgvs_p->setText(data_.hgvs_p);
	ui_.classification->setText(data_.classification);

	//phenotype data
	ui_.phenos->setPhenotypes(data_.phenos);
}

void LovdUploadDialog::guiToData()
{
	//sample data
	data_.processed_sample = ui_.processed_sample->text();
	data_.gender = ui_.gender->text();
	data_.genotype = ui_.genotype->text();

	//variant data
	data_.gene = ui_.gene->text();
	data_.nm_number = ui_.nm_number->text();
	data_.hgvs_g = ui_.hgvs_g->text();
	data_.hgvs_c = ui_.hgvs_c->text();
	data_.hgvs_p = ui_.hgvs_p->text();
	data_.classification = ui_.classification->text();

	//phenotype data
	data_.phenos = ui_.phenos->selectedPhenotypes();
}

void LovdUploadDialog::checkGuiData()
{
	//perform checks
	QStringList errors;
	if (ui_.gene->text().trimmed().isEmpty())
	{
		errors << "Gene empty!";
	}
	if (ui_.nm_number->text().trimmed().isEmpty())
	{
		errors << "Transcript empty!";
	}
	if (ui_.hgvs_g->text().trimmed().isEmpty())
	{
		errors << "HGVS.g empty!";
	}
	if (ui_.hgvs_c->text().trimmed().isEmpty())
	{
		errors << "HGVS.c empty!";
	}
	if (ui_.classification->text().trimmed().isEmpty())
	{
		errors << "classification empty!";
	}
	if (ui_.phenos->selectedPhenotypes().count()==0)
	{
		errors << "no phenotypes selected!";
	}

	//show error or enable upload button
	if (errors.count()>0)
	{
		ui_.upload_btn->setEnabled(false);
		ui_.comment_upload->setText("Cannot upload data because:\n  - " +  errors.join("\n  -"));
	}
	else
	{
		ui_.upload_btn->setEnabled(true);
		ui_.comment_upload->clear();

		//check if already published
		QString previsous_upload_data = db_.getVariantPublication(data_.processed_sample, data_.variant);
		if (previsous_upload_data!="")
		{
			ui_.comment_upload->setText("<font color='red'>WARNING: variant already uploaded!</font><br>" + previsous_upload_data);
		}
	}
}

void LovdUploadDialog::printResults()
{
	QPrinter printer(QPrinter::HighResolution);
	printer.setFullPage(true);
	QPrintDialog *dlg = new QPrintDialog(&printer, this);
	if (dlg->exec() == QDialog::Accepted)
	{
		QTextDocument *doc = new QTextDocument();
		doc->setPlainText(ui_.comment_upload->toPlainText());
		doc->print(&printer);
		delete doc;
	}
	delete dlg;
}

void LovdUploadDialog::updatePrintButton()
{
	ui_.print_btn->setEnabled(!ui_.comment_upload->toPlainText().trimmed().isEmpty());
}

QByteArray LovdUploadDialog::create(const LovdUploadData& data)
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
	stream << "                \"@id\": \"" << data.processed_sample.trimmed() << "\",\n";
	stream << "                \"gender\": {\n";
	stream << "                    \"@code\": \"" << convertGender(data.gender.trimmed()) <<"\"\n";
	stream << "                },\n";
	foreach(const Phenotype& pheno, data.phenos)
	{
		stream << "                \"phenotype\": [\n";
		stream << "                    {\n";
		stream << "                        \"@term\": \"" << pheno.name().trimmed() << "\",\n";
		stream << "                        \"@source\": \"HPO\",\n";
		stream << "                        \"@accession\": \"" << pheno.accession().mid(3).trimmed() << "\"\n";
		stream << "                    }\n";
		stream << "                ],\n";
	}

	//general variant info
	stream << "                \"variant\": [\n";
	stream << "                    {\n";
	stream << "                        \"@copy_count\": \"" << convertGenotype(data.genotype.trimmed()) << "\",\n";
	stream << "                        \"@type\": \"DNA\",\n";
	stream << "                        \"ref_seq\": {\n";
	stream << "                            \"@source\": \"genbank\",\n";
	stream << "                            \"@accession\": \"" << chromosomeToAccession(data.variant.chr()) << "\"\n"; //official identifier for hg19
	stream << "                        },\n";
	stream << "                        \"name\": {\n";
	stream << "                            \"@scheme\": \"HGVS\",\n";
	stream << "                            \"#text\": \"" << data.hgvs_g.trimmed() << "\"\n";
	stream << "                        },\n";
	stream << "                        \"pathogenicity\": {\n";
	stream << "                            \"@scope\": \"individual\",\n";
	stream << "                            \"@term\": \"" << convertClassification(data.classification.trimmed()) << "\"\n";
	stream << "                        },\n";
	stream << "                        \"variant_detection\": [\n";
	stream << "                            {\n";
	stream << "                                \"@template\": \"DNA\",\n";
	stream << "                                \"@technique\": \"SEQ\"\n";
	stream << "                            }\n";
	stream << "                        ],\n";
	stream << "                        \"seq_changes\": {\n";
	stream << "                            \"variant\": [\n";
	stream << "                                {\n";
	stream << "                                    \"@type\": \"cDNA\",\n";
	stream << "                                    \"gene\": {\n";
	stream << "                                        \"@source\": \"HGNC\",\n";
	stream << "                                        \"@accession\": \"" << data.gene.trimmed() << "\"\n";
	stream << "                                    },\n";
	stream << "                                    \"ref_seq\": {\n";
	stream << "                                        \"@source\": \"genbank\",\n";
	stream << "                                        \"@accession\": \"" << data.nm_number.trimmed() << "\"\n";
	stream << "                                    },\n";
	stream << "                                    \"name\": {\n";
	stream << "                                        \"@scheme\": \"HGVS\",\n";
	stream << "                                        \"#text\": \"" << data.hgvs_c.trimmed() << "\"\n";
	stream << "                                    },\n";
	stream << "                                    \"seq_changes\": {\n";
	stream << "                                        \"variant\": [\n";
	stream << "                                            {\n";
	stream << "                                                \"@type\": \"RNA\",\n";
	stream << "                                                \"name\": {\n";
	stream << "                                                    \"@scheme\": \"HGVS\",\n";
	stream << "                                                    \"#text\": \"r.(?)\"\n"; //we do not have HGVS.r info!
	stream << "                                                }";
	if (data.hgvs_p.trimmed()=="")
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
		stream << "                                                                \"#text\": \"" << data.hgvs_p.trimmed() << "\"\n";
		stream << "                                                            }\n";
		stream << "                                                        }\n";
		stream << "                                                    ]\n";
		stream << "                                                }\n";
	}
	stream << "                                            }\n";
	stream << "                                        ]\n";
	stream << "                                    }\n";
	stream << "                                }\n";

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

QString LovdUploadDialog::getSettings(QString key)
{
	QString output = Settings::string(key).trimmed();
	if (output.isEmpty())
	{
		THROW(FileParseException, "Settings INI file does not contain key '" + key + "'!");
	}
	return output;
}

QString LovdUploadDialog::convertClassification(QString classification)
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

	THROW(ProgrammingException, "Unknown classification '" + classification + "' in LovdUploadDialog::create(...) method!");
}

QString LovdUploadDialog::chromosomeToAccession(const Chromosome& chr)
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

	THROW(ProgrammingException, "Unknown chromosome '" + chr_str + "' in LovdUploadDialog::create(...) method!");
}

QString LovdUploadDialog::convertGender(QString gender)
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
	THROW(ProgrammingException, "Unknown gender '" + gender + "' in LovdUploadDialog::create(...) method!");
}

QString LovdUploadDialog::convertGenotype(QString genotype)
{
	if (genotype=="het")
	{
		return "1";
	}
	if (genotype=="hom")
	{
		return "2";
	}

	THROW(ProgrammingException, "Unknown genotype '" + genotype + "' in LovdUploadDialog::create(...) method!")
}
