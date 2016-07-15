#include "ApprovedGenesDialog.h"

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
	foreach(const QString& line, lines)
	{
		QByteArray line_trimmed = line.toLatin1().trimmed();
		if (!line_trimmed.isEmpty() && line_trimmed[0]!='#')
		{
			QList<QByteArray> parts = line_trimmed.split('\t');
			QPair<QByteArray, QByteArray> output = db.geneToApproved(parts[0]);
			ui_.text->appendPlainText(output.first + '\t' + output.second);
		}
	}
}
