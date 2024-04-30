#include "GHGAUploadDialog.h"
#include "Exceptions.h"
#include "NGSD.h"
#include "GenLabDB.h"

GHGAUploadDialog::GHGAUploadDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.btn_search, SIGNAL(clicked()), this, SLOT(search()));

	NGSD db;
	ui_.s_project_type->addItem("");
	ui_.s_project_type->addItems(db.getEnum("project", "type"));
	ui_.s_project_type->setCurrentText("diagnostic");
	ui_.s_system_type->addItem("");
	ui_.s_system_type->addItems(db.getEnum("processing_system", "type"));
	ui_.s_system_type->setCurrentText("WGS");
}

void GHGAUploadDialog::search()
{
	//init
	ui_.output->clear();

	//determine search paramters
	ProcessedSampleSearchParameters params;
	params.include_bad_quality_samples=false;
	params.run_finished=true;
	if (ui_.s_project_type->currentText()!="")
	{
		params.p_type = ui_.s_project_type->currentText();
	}
	if (ui_.s_system_type->currentText()!="")
	{
		params.sys_type = ui_.s_system_type->currentText();
	}
	if (ui_.s_tumor_status->currentText()=="no tumor")
	{
		params.include_tumor_samples = false;
	}
	if (ui_.s_tumor_status->currentText()=="tumor only")
	{
		params.include_germline_samples = false;
	}

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

			//determine processed samples for line
			QString selected_ps;
			QStringList ps_list = genlab.samplesWithSapID(sap_id, params);
			if (ps_list.count()==0)
			{
				selected_ps = "Skipped - No processed sample found for SAP id '" + sap_id +"'";
			}
			else if (ps_list.count()>1)
			{
				selected_ps = "Skipped - " + QString::number(ps_list.count()) + " processed samples found for SAP id '" + sap_id +"': " + ps_list.join(", ");
			}
			else
			{
				selected_ps = ps_list.first();
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
