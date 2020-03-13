#include "VariantConversionWidget.h"
#include "GUIHelper.h"
#include "VariantList.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Settings.h"
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
		ui_.input_label->setText("Input (VCF):");
		ui_.output_label->setText("Output (GSvar):");
		ui_.load_btn->setVisible(true);
	}
	else if (mode==HGVSC_TO_GSVAR)
	{
		ui_.input_label->setText("Input (HGVS.c):");
		ui_.output_label->setText("Output (GSvar):");
		ui_.load_btn->setVisible(false);
	}
}

void VariantConversionWidget::loadInputFromFile()
{
	//load
	try
	{
		QString path = Settings::path("path_variantlists");
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

	//convert
	try
	{
		QStringList lines = ui_.input->toPlainText().split("\n");
		QStringList output;
		foreach(QString line, lines)
		{
			line = line.trimmed();
			if (line=="" || line[0]=="#")
			{
				output << "";
			}
			else if (mode_==VCF_TO_GSVAR)
			{
				QStringList parts = line.split("\t");
				if (parts.count()<5) THROW(ArgumentException, "Invalid VCF variant '" + line + "' - too few tab-separated parts!");

				int start = Helper::toInt(parts[1], "VCF start position", line);
				Sequence ref_bases = parts[3].toLatin1().toUpper();
				int end = start + ref_bases.length()-1;
				Variant variant(parts[0], start, end, ref_bases, parts[4].toLatin1().toUpper());

				variant.normalize("-", true);

				output << variant.toString(true).replace(" ", "\t");
			}
			else if (mode_==HGVSC_TO_GSVAR)
			{
				//TODO
			}
		}

		ui_.output->setPlainText(output.join("\n"));
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Conversion error", e.message());
	}
}
