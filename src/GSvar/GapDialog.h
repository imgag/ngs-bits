#ifndef GAPDIALOG_H
#define GAPDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>
#include "BedFile.h"
#include "GapValidationLabel.h"

namespace Ui {
class GapDialog;
}

class GapDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GapDialog(QWidget* parent, QString sample_name, QString roi_file);
	~GapDialog();
	void process(QString bam_file, const BedFile& roi, QSet<QString> genes);
	QString report() const;

signals:
	void openRegionInIgv(QString region);

private slots:
	void gapDoubleClicked(QTableWidgetItem* item);

private:
	QTableWidgetItem* createItem(QString text, bool highlight = false, bool align_right = false);
	GapValidationLabel::State state(int row) const;
	QString gapAsTsv(int row) const;
	int gapSize(int row) const;

	QString sample_name_;
	QString roi_file_;
	Ui::GapDialog* ui;
};

#endif // GAPDIALOG_H
