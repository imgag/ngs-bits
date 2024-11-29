#include "SampleCountWidget.h"
#include "NGSD.h"
#include "GUIHelper.h"

SampleCountWidget::SampleCountWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);
}

void SampleCountWidget::delayedInitialization()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		NGSD db;
		QStringList groups = db.getEnum("sample", "disease_group");
		QStringList types = QStringList() << "lrGS" << "WGS" << "WES" << "Panel";

		//add row headers to table
		ui_.table->setRowCount(groups.count()+1);
		for (int r=0; r<groups.count(); ++r)
		{
			ui_.table->setVerticalHeaderItem(r, GUIHelper::createTableItem(groups[r]));
		}
		ui_.table->setVerticalHeaderItem(groups.count(), GUIHelper::createTableItem("sum"));

		//get counts
		QHash<QString, int> counts;
		SqlQuery query = db.getQuery();
		query.exec("SELECT ps.id, s.sample_type as s_type, sys.type as sys_type, s.disease_group FROM sample s, processed_sample ps, processing_system sys WHERE ps.quality!='bad' AND s.tumor=0 AND s.ffpe=0 AND s.id=ps.sample_id AND ps.processing_system_id=sys.id AND ps.id IN (SELECT DISTINCT processed_sample_id FROM detected_variant)");
		while(query.next())
		{
			if (!query.value("s_type").toString().startsWith("DNA")) continue;

			QString sys_type = query.value("sys_type").toString();
			if (sys_type.startsWith("Panel")) sys_type = "Panel";
			if (!types.contains(sys_type)) continue;

			QString tag = sys_type + " " + query.value("disease_group").toString();
			++counts[tag];
			++counts[sys_type];
		}

		//fill table
		for (int c=0; c<types.count(); ++c)
		{
			int sum = 0;
			for (int r=0; r<groups.count(); ++r)
			{
				QString tag = types[c] + " " + groups[r];
				int count = counts[tag];
				ui_.table->setItem(r, c, GUIHelper::createTableItem(QString::number(count), Qt::AlignRight|Qt::AlignTop));

				sum += count;
			}
			ui_.table->setItem(groups.count(), c, GUIHelper::createTableItem(QString::number(sum), Qt::AlignRight|Qt::AlignTop));
		}

		for (int r=0; r<groups.count(); ++r)
		{
			int sum = 0;
			for (int c=0; c<types.count(); ++c)
			{
				QString tag = types[c] + " " + groups[r];
				sum += counts[tag];
			}
			ui_.table->setItem(r, types.count(), GUIHelper::createTableItem(QString::number(sum), Qt::AlignRight|Qt::AlignTop));
		}

		//adapt widths
		GUIHelper::resizeTableCellHeightsToFirst(ui_.table);
		GUIHelper::resizeTableCellWidths(ui_.table);

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Sample counts initialization");
	}

}
