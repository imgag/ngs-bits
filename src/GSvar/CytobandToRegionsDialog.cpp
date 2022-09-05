#include "CytobandToRegionsDialog.h"
#include "NGSHelper.h"
#include "Exceptions.h"
#include "GSvarHelper.h"

CytobandToRegionsDialog::CytobandToRegionsDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.convert_btn, SIGNAL(clicked(bool)), this, SLOT(convert()));
}

void CytobandToRegionsDialog::convert()
{
	QStringList output;

	QStringList lines = ui_.in->toPlainText().split("\n");
	foreach(QString line, lines)
	{
		line = line.trimmed();

		QString line_out;
		if (!line.isEmpty())
		{
			try
			{
				line_out = NGSHelper::cytoBandToRange(GSvarHelper::build(), line.toUtf8()).toString(true);
			}
			catch(ArgumentException& e)
			{
				line_out = e.message();
			}
		}

		output << line_out;
	}

	ui_.out->setPlainText(output.join("\n"));
}
