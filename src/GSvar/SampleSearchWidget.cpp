#include "SampleSearchWidget.h"
#include "NGSD.h"

SampleSearchWidget::SampleSearchWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, db_()
	, is_valid_(true)
{
	ui_.setupUi(this);

	//init search criteria
	//sample
	ui_.s_name->fill(db_.createTable("sample", "SELECT id, name FROM sample"), true);
	ui_.s_species->fill(db_.createTable("species", "SELECT id, name FROM species"), true);
	ui_.s_quality->addItems(db_.getEnum("processed_sample", "quality"));
	//project
	ui_.p_name->fill(db_.createTable("project", "SELECT id, name FROM project"), true);
	//system
	ui_.sys_name->fill(db_.createTable("processing_system", "SELECT id, name_manufacturer FROM processing_system"), true);
	//run
	ui_.r_name->fill(db_.createTable("sequencing_run", "SELECT id, name FROM sequencing_run"), true);

	//signals/slots
	connect(ui_.search_btn, SIGNAL(clicked(bool)), this, SLOT(search()));
}

void SampleSearchWidget::search()
{
	checkValid();
	if (!is_valid_) return;

	//TODO
	qDebug() << __LINE__;
}

void SampleSearchWidget::checkValid()
{
	is_valid_ = true;

	QPalette palette;
	QPalette palette_invalid;
	palette_invalid.setColor(QPalette::Base, QColor(255, 150, 150));

	if (!ui_.s_name->text().isEmpty() && !ui_.s_name->isValidSelection())
	{
		ui_.s_name->setPalette(palette_invalid);
		is_valid_ = false;
	}
	else
	{
		ui_.s_name->setPalette(palette);
	}

	DBTable ps_table = db_.createTable("processed_sample", "SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) "
									   " FROM sample s, processed_sample ps "
									   " WHERE ps.sample_id=s.id");
	ui_.sample_table->setData(ps_table);
}
