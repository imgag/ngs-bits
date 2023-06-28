#include "GHGAUploadDialog.h"
#include "Exceptions.h"
#include "NGSD.h"
#include "GenLabDB.h"

GHGAUploadDialog::GHGAUploadDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.btn_test, SIGNAL(clicked()), this, SLOT(test()));
}

void GHGAUploadDialog::test()
{
	//init
	ui_.output->clear();
	ui_.btn_create->setEnabled(false);

	//process
	try
	{
		NGSD db;
		GenLabDB genlab;
		QStringList lines = ui_.samples->toPlainText().split("\n");
		foreach(QString line, lines)
		{
			//trim line
			while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
			if (line.isEmpty() || line[0]=='#') continue;

			//split and check data
			QStringList parts = line.split("\t");
			if (parts.count()==2) parts << "";
			if (parts.count()!=3) THROW(ArgumentException, "Sample line contains invalid number of elements:" + line);

			QString pseudonym = parts[0];
			QString sap_id = parts[1];
			QString system_type = parts[2];

			//determine processed samples for line
			QString selected_ps;
			QStringList ps_list = genlab.samplesWithSapID(sap_id, system_type);
			if (ps_list.count()==0)
			{
				selected_ps = "Skipped - No processed sample found for SAP id '" + sap_id +"'";
			}
			if (ps_list.count()>1)
			{
				selected_ps = "Skipped - " + QString::number(ps_list.count()) + " processed samples found for SAP id '" + sap_id +"'";
			}
			ui_.output->appendPlainText(pseudonym + "\t" + sap_id + "\t" + selected_ps);
			qApp->processEvents(); //update GUI
		}
	}
	catch (Exception& e)
	{
		ui_.output->appendPlainText("ERROR: "+e.message());
	}
}
