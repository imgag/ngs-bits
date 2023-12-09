#include "ImportDialog.h"
#include "Exceptions.h"
#include <QClipboard>
#include "VariantOpenDialog.h"
#include "GUIHelper.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include "GlobalServiceProvider.h"

ImportDialog::ImportDialog(QWidget* parent, Type type)
	: QDialog(parent)
	, ui_()
	, type_(type)
	, ref_genome_idx_(Settings::string("reference_genome"))
{
	ui_.setupUi(this);
	setupGUI();
}

void ImportDialog::openVariants()
{
	if (type_!=Type::VARIANTS) THROW(ProgrammingException, "Cannot call ImportDialog::openVariants if the type is not VARIANTS");

	VariantList variants;
	for(int i=0; i<ui_.table->rowCount(); ++i)
	{
		QString text = ui_.table->item(i, 1)->text();
		if (!text.startsWith("error:"))
		{
			variants.append(Variant::fromString(text));
		}
	}
	QString gsvar_file = Helper::tempFileName(".GSvar");
	GSvarHelper::annotate(variants, gsvar_file);

	//open GSvar
	GlobalServiceProvider::openGSvarFile(gsvar_file);
}

void ImportDialog::setupGUI()
{
	ui_.variant_options->setVisible(type_==Type::VARIANTS);

	if (type_==Type::VARIANTS)
	{
		setWindowTitle("Import variants");
		ui_.label->setText("Select format and paste variants into the table (one per line).");
		ui_.table->setColumnCount(4);
		ui_.table->setHorizontalHeaderLabels(QStringList() << "input" << "normalized variant" << "NGSD count" << "NGSD class");
	}
	else
	{
		THROW(ProgrammingException, "Unhandled type in ImportDialog::setTitleAndLabel");
	}
}

void ImportDialog::handlePaste(QStringList lines)
{
	ui_.table->setRowCount(0);
	NGSD db;

	if (type_==Type::VARIANTS)
	{
		QString format = VariantOpenDialog::selectedFormat(ui_.variant_options->layout());
		int row_index = 0;
		foreach(QString line, lines)
		{
			ui_.table->setRowCount(row_index + 1);

			Variant variant;
			QString error;
			VariantOpenDialog::parseVariant(format, line, ref_genome_idx_, variant, error);
			ui_.table->setItem(row_index, 0, GUIHelper::createTableItem(line));
			ui_.table->setItem(row_index, 1, GUIHelper::createTableItem(variant.isValid() ? variant.toString(QChar(), -1, true) : "error: " + error));
			QString var_id = db.variantId(variant, false);
			if (var_id.isEmpty())
			{
				ui_.table->setItem(row_index, 2, GUIHelper::createTableItem("no"));
			}
			else
			{
				ui_.table->setItem(row_index, 2, GUIHelper::createTableItem("yes"));
				ClassificationInfo class_info = db.getClassification(variant);
				ui_.table->setItem(row_index, 3, GUIHelper::createTableItem(class_info.classification));
			}
			++row_index;
		}
	}
	else
	{
		THROW(ProgrammingException, "Unhandled type in ImportDialog::handlePaste");
	}

	GUIHelper::resizeTableCells(ui_.table, 500, false);
}

void ImportDialog::keyPressEvent(QKeyEvent* e)
{
	if (e == QKeySequence::Paste)
	{
		QStringList lines;
		foreach(QString line, QApplication::clipboard()->text().split("\n"))
		{
			if (line.trimmed().isEmpty() || line[0]=='#') continue;

			//remove newline characters
			while(line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

			lines << line;
		}
		handlePaste(lines);
		e->accept();
		return;
	}

	QDialog::keyPressEvent(e);
}
