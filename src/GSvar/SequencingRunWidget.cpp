#include "SequencingRunWidget.h"
#include "ui_SequencingRunWidget.h"

SequencingRunWidget::SequencingRunWidget(QWidget* parent, QString run_id)
	: QWidget(parent)
	, ui_(new Ui::SequencingRunWidget)
	, run_id_(run_id)
{
	ui_->setupUi(this);

	updateGUI();
}

SequencingRunWidget::~SequencingRunWidget()
{
	delete ui_;
}

void SequencingRunWidget::updateGUI()
{
	//#### run details ####
	SqlQuery query = db_.getQuery();
	query.exec("SELECT r.*, d.name d_name, d.type d_type FROM sequencing_run r, device d WHERE r.device_id=d.id AND r.id='" + run_id_ + "'");
	query.next();
	ui_->name->setText(query.value("name").toString());
	ui_->fcid->setText(query.value("fcid").toString());
	ui_->start->setText(query.value("start_date").toString());
	ui_->end->setText(query.value("end_date").toString());
	ui_->recipe->setText(query.value("recipe").toString());
	ui_->comments->setText(query.value("comment").toString());
	ui_->device->setText(query.value("d_name").toString() + " (" + query.value("d_type").toString() + ")");
	ui_->molarity_and_method->setText(query.value("pool_molarity").toString() + " (" + query.value("pool_quantification_method").toString() + ")");
	ui_->quality->setText(query.value("quality").toString());
	ui_->status->setText(query.value("status").toString());

	//#### sample table ####

}
