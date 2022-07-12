#include "GapClosingEditDialog.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"

GapClosingEditDialog::GapClosingEditDialog(QWidget *parent, int id)
	: QDialog(parent)
	, ui_()
	, id_(id)
{
	ui_.setupUi(this);
	ui_.status->addItems(db_.getEnum("gaps", "status"));

	updateGUI();
}

void GapClosingEditDialog::updateGUI()
{
	SqlQuery query = db_.getQuery();
	query.exec("SELECT * FROM gaps WHERE id="+QString::number(id_));
	if (query.size()==0) return;
	query.next();

	//processed sample
	QString ps_id = query.value("processed_sample_id").toString();
	ui_.ps->setText(db_.processedSampleName(ps_id));

	//gap
	Chromosome chr = query.value("chr").toString();
	int start = query.value("start").toInt();
	int end = query.value("end").toInt();
	QString text = chr.str()+":"+QString::number(start)+"-"+QString::number(end);
	if (GSvarHelper::build()==GenomeBuild::HG38)
	{
		QString tmp;
		try
		{
			BedLine coords_hg19 = GSvarHelper::liftOver(chr, start, end, false);
			tmp = coords_hg19.toString(true);
		}
		catch(Exception& e)
		{
			tmp = e.message();
		}
		text += " // HG19: " + tmp;
	}
	ui_.gap->setText(text);

	//status
	ui_.status->setCurrentText(query.value("status").toString());

	//history:
	ui_.history->setPlainText(query.value("history").toString());
}

void GapClosingEditDialog::store()
{
	//update status
	db_.updateGapStatus(id_, ui_.status->currentText());
}
