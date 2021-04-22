#include "DBEditor.h"
#include "ProcessingSystemWidget.h"
#include "GSvarHelper.h"
#include "NGSD.h"
#include "GUIHelper.h"
#include "GlobalServiceProvider.h"
#include <QDir>
#include <QDesktopServices>
#include <QProcess>
#include <QDialog>

ProcessingSystemWidget::ProcessingSystemWidget(QWidget* parent, int sys_id)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
	, sys_id_(sys_id)
{
	ui_.setupUi(this);
	connect(ui_.update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_.igv_btn, SIGNAL(clicked(bool)), this, SLOT(openRoiInIGV()));
	connect(ui_.edit_btn, SIGNAL(clicked(bool)), this, SLOT(edit()));
}

void ProcessingSystemWidget::updateGUI()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//get variant id
	NGSD db;
	ProcessingSystemData ps_data = db.getProcessingSystemData(sys_id_);

	//###base infos###
	ui_.name_long->setText(ps_data.name);
	ui_.name_short->setText(ps_data.name_short);
	ui_.adapter1->setText(ps_data.adapter1_p5);
	ui_.adapter2->setText(ps_data.adapter2_p7);
	ui_.type->setText(ps_data.type);
	ui_.shotgun->setText(ps_data.shotgun ? "yes" : "no");
	ui_.umi->setText(ps_data.umi_type);
	ui_.genome->setText(ps_data.genome);

	//###target region infos###
	BedFile roi = GlobalServiceProvider::database().processingSystemRegions(sys_id_);
	if (!roi.isEmpty())
	{
		ui_.roi_bases->setText(QString::number(roi.baseCount(), 'f', 0));
		ui_.roi_regions->setText(QString::number(roi.count(), 'f', 0));
	}
	GeneSet roi_genes = GlobalServiceProvider::database().processingSystemGenes(sys_id_);
	if (!roi_genes.isEmpty())
	{
		ui_.roi_genes->setText(QString::number(roi_genes.count(), 'f', 0));
	}

	//###processed sample###
	QString samples = db.getValue("SELECT COUNT(id) FROM processed_sample WHERE processing_system_id=" + QString::number(sys_id_)).toString();
	ui_.number_of_samples_processed->setText(samples);

	QApplication::restoreOverrideCursor();
}

void ProcessingSystemWidget::delayedInitialization()
{
	updateGUI();
}

void ProcessingSystemWidget::edit()
{
	QString sys_name = ui_.name_long->text();

	DBEditor* widget = new DBEditor(this, "processing_system", sys_id_);
	auto dlg = GUIHelper::createDialog(widget, "Edit processing system " + sys_name ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateGUI();
	}
}

void ProcessingSystemWidget::openRoiInIGV()
{
	//load ROI
	BedFile roi = GlobalServiceProvider::database().processingSystemRegions(sys_id_);

	//store to temporary file
	QString roi_file = GSvarHelper::localRoiFolder() + ui_.name_short->text().trimmed() + ".bed";
	roi_file = Helper::canonicalPath(roi_file);
	roi.store(roi_file);

	emit executeIGVCommands(QStringList() << "load \"" + roi_file + "\"");
}
