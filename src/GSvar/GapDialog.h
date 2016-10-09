#ifndef GAPDIALOG_H
#define GAPDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>
#include "BedFile.h"

namespace Ui {
class GapDialog;
}

class GapDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GapDialog(QWidget* parent = 0);
	void setSampleName(QString sample_name);
	void process(QString bam_file, const BedFile& roi, QSet<QString> genes);
	~GapDialog();

signals:
	void openRegionInIgv(QString region);

private slots:
	void gapDoubleClicked(QTableWidgetItem* item);

private:
	QTableWidgetItem* createItem(QString text, bool highlight = false, bool align_right = false)
	{
		QTableWidgetItem* item = new QTableWidgetItem(text);
		if (highlight) item->setBackgroundColor(Qt::lightGray);
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
		if (align_right) item->setTextAlignment(Qt::AlignRight|Qt::AlignCenter);
		return item;
	}

	QString sample_name_;
	Ui::GapDialog* ui;
};

#endif // GAPDIALOG_H
