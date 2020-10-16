#include "VariantConversionWidget.h"
#include "GUIHelper.h"
#include "VariantList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Settings.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>

VariantConversionWidget::VariantConversionWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, mode_(NONE)
{
	ui_.setupUi(this);
	GUIHelper::styleSplitter(ui_.splitter);
	connect(ui_.convert_btn, SIGNAL(clicked(bool)), this, SLOT(convert()));
	connect(ui_.load_btn, SIGNAL(clicked(bool)), this, SLOT(loadInputFromFile()));
}

void VariantConversionWidget::setMode(VariantConversionWidget::ConversionMode mode)
{
	mode_ = mode;

	if (mode==VCF_TO_GSVAR)
	{
		ui_.message->setVisible(false);
		ui_.input_label->setText("Input (VCF):");
		ui_.output_label->setText("Output (GSvar):");
		ui_.load_btn->setVisible(true);
	}
	else if (mode==HGVSC_TO_GSVAR)
	{
		ui_.message->setVisible(true);
		ui_.message->setText("<font color='red'>Attention: This feature is experimental. Please report any errors!</font>");
		ui_.input_label->setText("Input (HGVS.c):");
		ui_.output_label->setText("Output (GSvar):");
		ui_.load_btn->setVisible(false);
	}
	else if (mode==GSVAR_TO_VCF)
	{
		ui_.message->setVisible(false);
		ui_.input_label->setText("Input (GSvar):");
		ui_.output_label->setText("Output (VCF):");
		ui_.load_btn->setVisible(false);
	}
}

void VariantConversionWidget::loadInputFromFile()
{
	//load
	try
	{
		QString path = Settings::path("path_variantlists", true);
		QString filename = QFileDialog::getOpenFileName(this, "Select target region file", path, "VCF files (*.vcf);;All files (*.*)");
		if (filename=="") return;

		ui_.input->clear();

		QStringList lines = Helper::loadTextFile(filename, true, '#', true);
		ui_.input->setPlainText(lines.join("\n"));
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "File error", e.message());
	}

	//auto-convert
	convert();
}

void VariantConversionWidget::convert()
{
	//clear
	ui_.output->clear();

	//load reference genome
	FastaFileIndex ref_genome_idx(Settings::string("reference_genome"));

	//convert
	try
	{
		QStringList lines = ui_.input->toPlainText().split("\n");
		QStringList output;
		if (mode_==VCF_TO_GSVAR)
		{
			foreach(QString line, lines)
			{
				line = line.trimmed();
				if (line=="" || line[0]=="#")
				{
					output << "";
					continue;
				}

				QStringList parts = line.split("\t");
				if (parts.count()<5) THROW(ArgumentException, "Invalid VCF variant '" + line + "' - too few tab-separated parts!");

				int start = Helper::toInt(parts[1], "VCF start position", line);
				Sequence ref_bases = parts[3].toLatin1().toUpper();
				int end = start + ref_bases.length()-1;

				Variant variant(parts[0], start, end, ref_bases, parts[4].toLatin1().toUpper());
				variant.normalize("-", true);

				variant.checkValid();
				if (variant.ref()!="-") variant.checkReferenceSequence(ref_genome_idx);

				output << variant.toString(true, -1, true).replace(" ", "\t");
			}
		}
		else if (mode_==HGVSC_TO_GSVAR)
		{
			NGSD db;

			foreach(QString line, lines)
			{
				line = line.trimmed();
				if (line=="" || line[0]=="#")
				{
					output << "";
					continue;
				}

				int sep_pos = line.indexOf(':');
				if (sep_pos==-1) THROW(ArgumentException, "Invalid HGVS.c variant '" + line + "' - the format is [transcipt name]:[variant]");
				QString transcript_name = line.left(sep_pos);
				QString hgvs_c = line.mid(sep_pos+1);

				int trans_id = db.transcriptId(transcript_name, false);
				if (trans_id==-1) //not found > try to match CCDS/RefSeq toEnsembl
				{
					//remove version number (if present)
					if (transcript_name[transcript_name.length()-2]=='.')
					{
						transcript_name = transcript_name.left(transcript_name.length()-2);
					}

					QString transcript_name2 = "";
					const QMap<QByteArray, QByteArrayList>& matches = GSvarHelper::transcriptMatches();
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
				if (trans_id==-1) THROW(ArgumentException, "Cannot determine Ensembl transcript for CCDS/RefSeq/Ensembl transcript identifier '" + transcript_name + "'!");
				Transcript transcript = db.transcript(trans_id);
				Variant variant = transcript.hgvsToVariant(hgvs_c, ref_genome_idx);

				variant.checkValid();
				if (variant.ref()!="-") variant.checkReferenceSequence(ref_genome_idx);

				output << variant.toString(true, -1, true).replace(" ", "\t");
			}
		}
		else if (mode_==GSVAR_TO_VCF)
		{
			FastaFileIndex idx(Settings::string("reference_genome"));

			foreach(QString line, lines)
			{
				line = line.trimmed();
				if (line=="" || line[0]=="#")
				{
					output << "";
					continue;
				}

				QStringList parts = line.split("\t");
				if (parts.count()<5) THROW(ArgumentException, "Invalid GSvar variant '" + line + "' - too few tab-separated parts!");

				int start = Helper::toInt(parts[1], "GSvar start position", line);
				int end = Helper::toInt(parts[2], "GSvar end position", line);
				Sequence ref = parts[3].toLatin1().toUpper().trimmed();
				Sequence obs = parts[4].toLatin1().toUpper().trimmed();

				Variant variant(parts[0], start, end, ref, obs);
				output << variant.toVCF(idx);
			}
		}

		ui_.output->setPlainText(output.join("\n"));
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Conversion error", e.message());
	}
}

