#include "ProjectWidget.h"
#include "NGSD.h"
#include "DiagnosticStatusOverviewDialog.h"

ProjectWidget::ProjectWidget(QWidget* parent, QString name)
	: QWidget(parent)
	, init_timer_(this, true)
	, name_(name)
{
	ui_.setupUi(this);
	connect(ui_.sample_overview, SIGNAL(linkActivated(QString)), this, SIGNAL(openProcessedSampleTab(QString)));
	connect(ui_.diag_btn, SIGNAL(clicked(bool)), this, SLOT(showDiagnosticStatusDialog()));
}

void ProjectWidget::delayedInitialization()
{
	updateGUI();
}

void ProjectWidget::updateGUI()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//### base infos
	NGSD db;
	SqlQuery query = db.getQuery();
	query.exec("SELECT p.*, u.name as coordinator FROM project p, user u WHERE p.internal_coordinator_id=u.id AND p.name='" + name_ + "'");
	query.next();
	ui_.name->setText(name_);
	ui_.aliases->setText(query.value("aliases").toString());
	ui_.type->setText(query.value("type").toString());
	ui_.analysis->setText(query.value("analysis").toString());
	ui_.preserve_fastqs->setText(query.value("preserve_fastqs").toString()=="1" ? "yes" : "no");
	ui_.coordinator->setText(query.value("coordinator").toString());
	QStringList emails = query.value("email_notification").toString().split(";");
	std::for_each(emails.begin(), emails.end(), [](QString& email){
																	email = email.trimmed();
																	email = "<a href='mailto:" + email + "'>" + email + "</a>";
																  });
	ui_.email_notification->setText(emails.join("; "));
	ui_.comment->setText(query.value("comment").toString());

	//### samples
	QString project_id = query.value("id").toString();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps, sys.name_manufacturer as sys FROM processing_system sys, processed_sample ps, sample s WHERE ps.sample_id=s.id AND sys.id=ps.processing_system_id AND ps.project_id='"+project_id+"' ORDER BY s.name ASC, ps.process_id ASC");
	ui_.num_samples->setText(QString::number(query.size()));

	QMap<QString, QStringList> samples_by_sys;
	while(query.next())
	{
		QString sys = query.value("sys").toString();
		QString ps = query.value("ps").toString();
		samples_by_sys[sys] << ("<a href='" + ps + "'>" + ps + "</a>");
	}
	QString sample_summary;
	for (auto it=samples_by_sys.begin(); it!=samples_by_sys.end(); ++it)
	{
		sample_summary += it.key() + " (" + QString::number(it.value().count()) + "):<br>";
		sample_summary += it.value().join(" ") + "<br><br>";
	}
	ui_.sample_overview->setText(sample_summary.trimmed());

	QApplication::restoreOverrideCursor();
}

void ProjectWidget::showDiagnosticStatusDialog()
{
	DiagnosticStatusOverviewDialog* dlg = new DiagnosticStatusOverviewDialog(this, name_);
	connect(dlg, SIGNAL(openProcessedSampleTab(QString)), this, SIGNAL(openProcessedSampleTab(QString)));
	dlg->exec();
}
