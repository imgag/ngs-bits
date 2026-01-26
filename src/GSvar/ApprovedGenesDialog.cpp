#include "ApprovedGenesDialog.h"
#include "NGSD.h"

ApprovedGenesDialog::ApprovedGenesDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
}

void ApprovedGenesDialog::on_convertBtn_pressed()
{
	NGSD db;

	QStringList lines = ui_.text->toPlainText().split('\n');
	ui_.text->clear();
	foreach(QString line, lines)
	{
		line = line.trimmed();
		if (!line.isEmpty() && line[0]!='#')
		{
			QList<QString> parts = line.split('\t');
			QPair<QString, QString> output = db.geneToApprovedWithMessage(parts[0]);
			ui_.text->appendPlainText(output.first + '\t' + output.second);
		}
	}
}
