#ifndef VARIANTSHEETDIALOG_H
#define VARIANTSHEETDIALOG_H

#include <NGSD.h>
#include <QDialog>

namespace Ui {
class EvaluationSheetEditDialog;
}

class EvaluationSheetEditDialog : public QDialog
{
	Q_OBJECT

public:
	explicit EvaluationSheetEditDialog(QWidget *parent = 0);
	~EvaluationSheetEditDialog();
	void importEvaluationSheetData(EvaluationSheetData& evaluation_sheet_data);

signals:
	void EvaluationSheetDataUpdated();

protected slots:
	void updateEvaluationSheetData();
	void checkReviewer();
	void checkNonEmpty();

private:
	void initReviewerNames();
	Ui::EvaluationSheetEditDialog *ui_;
	EvaluationSheetData* evaluation_sheet_data_;
	mutable NGSD db_;
	DBTable users_;
};

#endif // EVALUATIONSHEETEDITDIALOG_H
