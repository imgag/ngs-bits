#include "LovdUploadDialog.h"
#include "HttpHandler.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPrinter>
#include <QPrintDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMenu>
#include <QMessageBox>

LovdUploadDialog::LovdUploadDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);

	connect(ui_.upload_btn, SIGNAL(clicked(bool)), this, SLOT(upload()));

	connect(ui_.processed_sample, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.chr, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.gene, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.nm_number, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_c, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_g, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_p, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.genotype, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.classification, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_c2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_g2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.hgvs_p2, SIGNAL(textEdited(QString)), this, SLOT(checkGuiData()));
	connect(ui_.genotype2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.classification2, SIGNAL(currentTextChanged(QString)), this, SLOT(checkGuiData()));
	connect(ui_.phenos, SIGNAL(phenotypeSelectionChanged()), this, SLOT(checkGuiData()));

	//update GUI elements for 2nd variant (only for free mode)
	connect(ui_.genotype, SIGNAL(currentTextChanged(QString)), this, SLOT(updateSecondVariantGui()));
	connect(ui_.genotype2, SIGNAL(currentTextChanged(QString)), this, SLOT(updateSecondVariantGui()));

	connect(ui_.print_btn, SIGNAL(clicked(bool)), this, SLOT(printResults()));
	connect(ui_.comment_upload, SIGNAL(textChanged()), this, SLOT(updatePrintButton()));
}

void LovdUploadDialog::setData(LovdUploadData data)
{
	//supported LOVD transcripts
	QStringList supported_transcripts = Helper::loadTextFile(":/Resources/lovd_transcripts.tsv", true, '#', true);

	//sample data
	ui_.processed_sample->setText(data.processed_sample);
	ui_.gender->setCurrentText(data.gender);

	ui_.processed_sample->setEnabled(false);
	ui_.gender->setEnabled(false);

	//variant data (variant 1)
	variant1_ = data.variant;
	QByteArray chr = data.variant.chr().str();
	if (chr=="chrMT") chr = "chrM";
	ui_.chr->setCurrentText(chr);
	ui_.gene->setText(data.gene);
	ui_.nm_number->setText(data.nm_number);
	ui_.hgvs_g->setText(data.hgvs_g);
	ui_.hgvs_c->setText(data.hgvs_c);
	ui_.hgvs_p->setText(data.hgvs_p);
	ui_.classification->setCurrentText(data.classification);
	ui_.genotype->setCurrentText(data.genotype);
	if (!data.trans_data.isEmpty())
	{
		QMenu* menu = new QMenu(this);
		for (int i=0; i<data.trans_data.count(); ++i)
		{
			const VariantTranscript& trans = data.trans_data[i];
			QAction* action = menu->addAction(trans.id + ": " + trans.gene + " / " + trans.type + " / " + trans.hgvs_c + " / " + trans.hgvs_p, this, SLOT(setTranscriptInfoVariant1()));
			action->setData(i);
			if (supported_transcripts.contains(trans.id))
			{
				action->setIcon(QIcon(":/Icons/LOVD.png"));
			}
		}
		ui_.refseq_btn->setMenu(menu);
	}
	ui_.chr->setEnabled(false);
	ui_.classification->setEnabled(false);
	ui_.genotype->setEnabled(false);

	//variant data (variant 2)
	if(data.variant2.isValid())
	{
		variant2_ = data.variant2;
		ui_.hgvs_g2->setText(data.hgvs_g2);
		ui_.hgvs_c2->setText(data.hgvs_c2);
		ui_.hgvs_p2->setText(data.hgvs_p2);
		ui_.classification2->setCurrentText(data.classification2);
		ui_.genotype2->setCurrentText(data.genotype2);
		if (!data.trans_data2.isEmpty())
		{
			QMenu* menu = new QMenu(this);
			for (int i=0; i<data.trans_data2.count(); ++i)
			{
				const VariantTranscript& trans = data.trans_data2[i];
				QAction* action = menu->addAction(trans.id + ": " + trans.gene + " / " + trans.type + " / " + trans.hgvs_c + " / " + trans.hgvs_p, this, SLOT(setTranscriptInfoVariant2()));
				action->setData(i);
				if (supported_transcripts.contains(trans.id))
				{
					action->setIcon(QIcon(":/Icons/LOVD.png"));
				}
			}
			ui_.refseq_btn2->setMenu(menu);
		}

		ui_.hgvs_g2->setEnabled(true);
		ui_.hgvs_c2->setEnabled(true);
		ui_.hgvs_p2->setEnabled(true);
		ui_.refseq_btn2->setEnabled(true);
	}

	ui_.genotype2->setEnabled(false);
	ui_.classification2->setEnabled(false);

	//if not in free mode => GUI of 2nd variant needs no updates
	disconnect(this, SLOT(updateSecondVariantGui()));

	//phenotype data
	ui_.phenos->setPhenotypes(data.phenos);

	data_ = data;
}

void LovdUploadDialog::upload()
{
	//init
	ui_.upload_btn->setEnabled(false);

	//create JSON-formatted upload data
	QByteArray upload_file = createJson();

	//upload data
	static HttpHandler http_handler(HttpRequestHandler::INI); //static to allow caching of credentials
	try
	{
		//add headers
		HttpHeaders add_headers;
		add_headers.insert("Content-Type", "application/json");
		add_headers.insert("Content-Length", QByteArray::number(upload_file.count()));

		//post request
		QString reply = http_handler.post("https://databases.lovd.nl/shared/api/v1/submissions", upload_file, add_headers);
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
			QString processed_sample = ui_.processed_sample->text().trimmed();

			QStringList details;
			details << "gene=" + ui_.gene->text();
			details << "transcript=" + ui_.nm_number->text();

			details << "hgvs_g=" + ui_.hgvs_g->text();
			details << "hgvs_c=" + ui_.hgvs_c->text();
			details << "hgvs_p=" + ui_.hgvs_p->text();
			details << "genotype=" + ui_.genotype->currentText();
			details << "classification=" + ui_.classification->currentText();

			if (isCompHet())
			{
				details << "hgvs_g2=" + ui_.hgvs_g2->text();
				details << "hgvs_c2=" + ui_.hgvs_c2->text();
				details << "hgvs_p2=" + ui_.hgvs_p2->text();
				details << "genotype2=" + ui_.genotype2->currentText();
				details << "classification2=" + ui_.classification2->currentText();
			}

			foreach(const Phenotype& pheno, ui_.phenos->selectedPhenotypes())
			{
				details << "phenotype=" + pheno.accession() + " - " + pheno.name();
			}

			//Upload only if variant(s) are set
			if (variant1_.isValid())
			{
				db_.addVariantPublication(processed_sample, variant1_, "LOVD", ui_.classification->currentText(), details.join(";"));
			}
			if (variant2_.isValid())
			{
				db_.addVariantPublication(processed_sample, variant2_, "LOVD", ui_.classification2->currentText(), details.join(";"));
			}

			//show result
			QStringList lines;
			lines << "DATA UPLOAD TO LOVD SUCCESSFUL";
			lines << "";
			lines << messages.join("\n");
			lines << "";
			lines << "sample: " + processed_sample;
			lines << "user: " + Helper::userName();
			lines << "date: " + Helper::dateTime();
			lines << "";
			lines << details;

			ui_.comment_upload->setText(lines.join("\n").replace("=", ": "));

			//write report file to transfer folder
			QString gsvar_publication_folder = Settings::path("gsvar_publication_folder");
			if (gsvar_publication_folder!="")
			{
				QString file_rep = gsvar_publication_folder + "/" + processed_sample + "_LOVD_" + QDate::currentDate().toString("yyyyMMdd") + ".txt";
				Helper::storeTextFile(file_rep, ui_.comment_upload->toPlainText().split("\n"));
			}
		}
		else
		{
			ui_.comment_upload->setText("DATA UPLOAD ERROR:\n" + messages.join("\n"));
			ui_.upload_btn->setEnabled(true);
		}
	}
	catch(Exception e)
	{
		ui_.comment_upload->setText("DATA UPLOAD FAILED:\n" + e.message());
		ui_.upload_btn->setEnabled(true);
	}
}

void LovdUploadDialog::checkGuiData()
{
	//check if already published
	if (ui_.processed_sample->text()!="" && variant1_.isValid())
	{
		QString upload_details = db_.getVariantPublication(ui_.processed_sample->text(), variant1_);
		if (upload_details!="")
		{
			ui_.upload_btn->setEnabled(false);
			ui_.comment_upload->setText("<font color='red'>ERROR: variant already uploaded!</font><br>" + upload_details);
			return;
		}
	}

	//perform checks
	QStringList errors;
	if (ui_.processed_sample->text().trimmed().isEmpty())
	{
		errors << "Processed sample unset!";
	}
	if (ui_.chr->currentText().trimmed().isEmpty())
	{
		errors << "Chromosome unset!";
	}
	if (ui_.gene->text().trimmed().isEmpty())
	{
		errors << "Gene unset!";
	}
	if (ui_.nm_number->text().trimmed().isEmpty())
	{
		errors << "Transcript unset!";
	}
	if (ui_.hgvs_g->text().trimmed().isEmpty())
	{
		errors << "HGVS.g unset!";
	}
	if (!ui_.hgvs_g->text().trimmed().isEmpty() && !ui_.hgvs_g->text().trimmed().startsWith("g."))
	{
		errors << "HGVS.g must start with 'g.'!";
	}
	if (ui_.hgvs_c->text().trimmed().isEmpty())
	{
		errors << "HGVS.c unset!";
	}
	if (!ui_.hgvs_c->text().trimmed().isEmpty() && !ui_.hgvs_c->text().trimmed().startsWith("c."))
	{
		errors << "HGVS.c must start with 'c.'!";
	}
	if (!ui_.hgvs_p->text().trimmed().isEmpty() && !ui_.hgvs_p->text().trimmed().startsWith("p."))
	{
		errors << "HGVS.p must start with 'p.'!";
	}
	if (ui_.genotype->currentText().trimmed().isEmpty())
	{
		errors << "Genotype unset!";
	}
	if (ui_.classification->currentText().trimmed().isEmpty())
	{
		errors << "Classification unset!";
	}

	if (isCompHet())
	{
		if (ui_.hgvs_g2->text().trimmed().isEmpty())
		{
			errors << "HGVS.g unset (variant 2)!";
		}
		if (!ui_.hgvs_g2->text().trimmed().isEmpty() && !ui_.hgvs_g2->text().trimmed().startsWith("g."))
		{
			errors << "HGVS.g must start with 'g.' (variant 2)!";
		}
		if (ui_.hgvs_c2->text().trimmed().isEmpty())
		{
			errors << "HGVS.c unset (variant 2)!";
		}
		if (!ui_.hgvs_c2->text().trimmed().isEmpty() && !ui_.hgvs_c2->text().trimmed().startsWith("c."))
		{
			errors << "HGVS.c must start with 'c.' (variant 2)!";
		}
		if (!ui_.hgvs_p2->text().trimmed().isEmpty() && !ui_.hgvs_p2->text().trimmed().startsWith("p."))
		{
			errors << "HGVS.p must start with 'p.' (variant 2)!";
		}
		if (ui_.genotype->currentText()!="het" || ui_.genotype2->currentText()!="het")
		{
			errors << "Two variants for upload, but they are not compound-heterozygous!";
		}
		if (ui_.classification2->currentText().trimmed().isEmpty())
		{
			errors << "Classification unset (variant 2)!";
		}
	}

	if (ui_.phenos->selectedPhenotypes().count()==0)
	{
		errors << "No phenotypes selected!";
	}

	//show error or enable upload button
	if (errors.count()>0)
	{
		ui_.upload_btn->setEnabled(false);
		ui_.comment_upload->setText("Cannot upload data because:\n  - " +  errors.join("\n  - "));
	}
	else
	{
		ui_.upload_btn->setEnabled(true);
		ui_.comment_upload->clear();
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

void LovdUploadDialog::updateSecondVariantGui()
{
	bool enabled = ui_.genotype->currentText()=="het" && ui_.genotype2->currentText()=="het";

	ui_.hgvs_g2->setEnabled(enabled);
	ui_.hgvs_c2->setEnabled(enabled);
	ui_.hgvs_p2->setEnabled(enabled);
	ui_.classification2->setEnabled(enabled);
	ui_.refseq_btn2->setEnabled(enabled);
}

void LovdUploadDialog::setTranscriptInfoVariant1()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr) THROW(ProgrammingException, "This should not happen!");

	bool ok = false;
	int index = action->data().toInt(&ok);
	if (!ok) THROW(ProgrammingException, "This should not happen!");

	const VariantTranscript& trans = data_.trans_data[index];
	ui_.gene->setText(trans.gene);
	ui_.nm_number->setText(trans.id);
	ui_.hgvs_c->setText(trans.hgvs_c);
	ui_.hgvs_p->setText(trans.hgvs_p);

	checkGuiData();
}

void LovdUploadDialog::setTranscriptInfoVariant2()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr) THROW(ProgrammingException, "This should not happen!");

	bool ok = false;
	int index = action->data().toInt(&ok);
	if (!ok) THROW(ProgrammingException, "This should not happen!");

	//check same transcript
	const VariantTranscript& trans = data_.trans_data2[index];
	if (trans.id!=ui_.nm_number->text())
	{
		QMessageBox::warning(this, "Transcript mismatch error", ui_.nm_number->text() + " selected as transcript for variant 1.\n" + trans.id + " selected as transcript for variant 2.\n\nThey do not match!");
		return;
	}

	ui_.hgvs_c2->setText(trans.hgvs_c);
	ui_.hgvs_p2->setText(trans.hgvs_p);

	checkGuiData();
}

bool LovdUploadDialog::isCompHet() const
{
	return variant2_.isValid() || ui_.genotype2->currentText()=="het";
}

QByteArray LovdUploadDialog::createJson()
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
	stream << "                \"@id\": \"" << ui_.processed_sample->text().trimmed() << "\",\n";
	stream << "                \"gender\": {\n";
	stream << "                    \"@code\": \"" << convertGender(ui_.gender->currentText().trimmed()) <<"\"\n";
	stream << "                },\n";
	stream << "                \"phenotype\": [\n";
	PhenotypeList phenotypes = ui_.phenos->selectedPhenotypes();
	for (int i=0; i<phenotypes.count(); ++i)
	{
		stream << "                    {\n";
		stream << "                        \"@term\": \"" << phenotypes[i].name().trimmed() << "\",\n";
		stream << "                        \"@source\": \"HPO\",\n";
		stream << "                        \"@accession\": \"" << phenotypes[i].accession().mid(3).trimmed() << "\"\n";
		stream << "                    }";
		if (i<phenotypes.count()-1) stream << ",";
		stream << "\n";
	}
	stream << "                ],\n";

	//variant info
	stream << "                \"variant\": [\n";
	QString chr = ui_.chr->currentText().trimmed();
	QString gene = ui_.gene->text().trimmed();
	QString transcript = ui_.nm_number->text().trimmed();
	createJsonForVariant(stream, chr, gene, transcript, ui_.hgvs_g, ui_.hgvs_c, ui_.hgvs_p, ui_.genotype, ui_.classification);
	if (isCompHet())
	{
		stream << ",\n";
		createJsonForVariant(stream, chr, gene, transcript, ui_.hgvs_g2, ui_.hgvs_c2, ui_.hgvs_p2, ui_.genotype, ui_.classification);
	}
	stream << "\n";

	//close all brackets
	stream << "                ]\n";
	stream << "            }\n";
	stream << "        ]\n";
	stream << "    }\n";
	stream << "}\n";
	stream << "\n";

	return output;
}

void LovdUploadDialog::createJsonForVariant(QTextStream& stream, QString chr, QString gene, QString transcript, QLineEdit* hgvs_g, QLineEdit* hgvs_c, QLineEdit* hgvs_p, QComboBox* genotype, QComboBox* classification)
{
	stream << "                    {\n";
	stream << "                        \"@copy_count\": \"" << convertGenotype(genotype->currentText().trimmed()) << "\",\n";
	stream << "                        \"@type\": \"DNA\",\n";
	stream << "                        \"ref_seq\": {\n";
	stream << "                            \"@source\": \"genbank\",\n";
	stream << "                            \"@accession\": \"" << chromosomeToAccession(chr) << "\"\n"; //official identifier for hg19
	stream << "                        },\n";
	stream << "                        \"name\": {\n";
	stream << "                            \"@scheme\": \"HGVS\",\n";
	stream << "                            \"#text\": \"" << hgvs_g->text().trimmed() << "\"\n";
	stream << "                        },\n";
	stream << "                        \"pathogenicity\": {\n";
	stream << "                            \"@scope\": \"individual\",\n";
	stream << "                            \"@term\": \"" << convertClassification(classification->currentText().trimmed()) << "\"\n";
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
	stream << "                                        \"@accession\": \"" << gene << "\"\n";
	stream << "                                    },\n";
	stream << "                                    \"ref_seq\": {\n";
	stream << "                                        \"@source\": \"genbank\",\n";
	stream << "                                        \"@accession\": \"" << transcript << "\"\n";
	stream << "                                    },\n";
	stream << "                                    \"name\": {\n";
	stream << "                                        \"@scheme\": \"HGVS\",\n";
	stream << "                                        \"#text\": \"" << hgvs_c->text().trimmed() << "\"\n";
	stream << "                                    },\n";
	stream << "                                    \"seq_changes\": {\n";
	stream << "                                        \"variant\": [\n";
	stream << "                                            {\n";
	stream << "                                                \"@type\": \"RNA\",\n";
	stream << "                                                \"name\": {\n";
	stream << "                                                    \"@scheme\": \"HGVS\",\n";
	stream << "                                                    \"#text\": \"r.(?)\"\n"; //we do not have HGVS.r info!
	stream << "                                                }";
	if (hgvs_p->text().trimmed()=="")
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
		stream << "                                                                \"#text\": \"" << hgvs_p->text().trimmed() << "\"\n";
		stream << "                                                            }\n";
		stream << "                                                        }\n";
		stream << "                                                    ]\n";
		stream << "                                                }\n";
	}
	stream << "                                            }\n";
	stream << "                                        ]\n";
	stream << "                                    }\n";
	stream << "                                }";
	stream << "                            ]\n";
	stream << "                        }\n";
	stream << "                    }\n";
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
