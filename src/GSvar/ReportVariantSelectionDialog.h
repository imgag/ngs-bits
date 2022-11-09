#ifndef REPORTVARIANTSELECTIONDIALOG_H
#define REPORTVARIANTSELECTIONDIALOG_H

#include <BedpeFile.h>
#include <CnvList.h>
#include <QDialog>
#include <ReportConfiguration.h>
#include <VariantList.h>

namespace Ui {
class ReportVariantSelectionDialog;
}

class ReportVariantSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ReportVariantSelectionDialog(QString ps_id, QWidget *parent = 0);
	~ReportVariantSelectionDialog();

private:
	Ui::ReportVariantSelectionDialog *ui_;
	QString ps_id_;
	int rc_id_;
	QSharedPointer<ReportConfiguration> report_config_;
	ReportVariantConfiguration selected_variant;

	void initTable();
};

#endif // REPORTVARIANTSELECTIONDIALOG_H
