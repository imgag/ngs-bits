#include "CnvSearchWidget.h"
#include "Exceptions.h"
#include "Chromosome.h"
#include "Helper.h"
#include <QMessageBox>

CnvSearchWidget::CnvSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(search()));

	ui_.coordinates->setText("chr17:57297834-57351873"); //TODO
}

void CnvSearchWidget::search()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	try
	{
		//parse
		QString coords = ui_.coordinates->text();
		QStringList parts = coords.replace("-", ":").split(":");
		if (parts.count()!=3) THROW(ArgumentException, "Could not split coordinates in three parts! " + QString::number(parts.count()) + " parts found.");

		Chromosome chr(parts[0]);
		if (!chr.isValid()) THROW(ArgumentException, "Invalid chromosome given: " + parts[0]);
		int start = Helper::toInt(parts[1], "Start cooridinate");
		int end = Helper::toInt(parts[2], "End cooridinate");

		//search matching CNVs
		QString query_str = "SELECT c.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as processed_sample, ps.quality as ps_quality, sys.name_manufacturer, c.chr, c.start, c.end, c.cn, c.quality_metrics as cnv_metrics, cs.caller, cs.quality_metrics as callset_metrics, cs.quality as callset_quality, s.disease_group, s.disease_status "
							"FROM cnv c, cnv_callset cs, processed_sample ps, processing_system sys, sample s "
							"WHERE s.id=ps.sample_id AND sys.id=ps.processing_system_id AND c.cnv_callset_id=cs.id AND ps.id=cs.processed_sample_id ";
		QString operation = ui_.operation->currentText();
		if (operation=="overlaps")
		{
			query_str += " AND chr='" + chr.strNormalized(true) + "' AND ((" + QString::number(start) + ">=start AND " + QString::number(start) + "<=end) OR (start>=" + QString::number(start) + " AND start<=" + QString::number(end) + "))";

		}
		else if (operation=="contains")
		{
			query_str += " AND chr='" + chr.strNormalized(true) + "' AND start<=" + QString::number(start) + " AND end>=" + QString::number(end);
		}
		else THROW(ProgrammingException, "Invalid operation: " + operation);

		//show samples with CNVs in table
		DBTable table = db_.createTable("cnv", query_str);
		ui_.table->setData(table);

		//TODO: HPO, cnv class from report config (color)
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "CNV search", "Error: Search could not be performed:\n" + e.message());
	}

	QApplication::restoreOverrideCursor();
}
