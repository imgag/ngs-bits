#ifndef GAPDIALOG_H
#define GAPDIALOG_H

#include <QDialog>
#include <QTableWidgetItem>
#include "BedFile.h"
#include "GapValidationLabel.h"
#include "GeneSet.h"

namespace Ui {
class GapDialog;
}

class GapDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GapDialog(QWidget* parent, QString sample_name, QString roi_file);
	~GapDialog();
	void process(QString bam_file, const BedFile& roi, const GeneSet& genes);
	QString report() const;

signals:
	void openRegionInIGV(QString region);

private slots:
	void gapDoubleClicked(QTableWidgetItem* item);
	void updateGeneFilter(QString text);

private:
	QTableWidgetItem* createItem(QString text, bool highlight = false, bool align_right = false);
	GapValidationLabel::State state(int row) const;
	void reportSection(QTextStream& stream, bool ccds_only) const;

	QString sample_name_;
	QString roi_file_;
	struct GapInfo
	{
		BedLine line;
		double avg_depth;
		GeneSet genes;
		BedLine ccds_overlap;

		QString asTsv(bool ccds_only) const
		{
			QString output = (ccds_only ? ccds_overlap.toString(false) : line.toString(false)) + "\t";
			output += "avg_depth=" + QString::number(avg_depth, 'f', 2);
			if (!genes.isEmpty()) output += " gene=" + genes.join(",");
			if (!ccds_only && isExonicSplicing()) output += " exonic/splicing";

			return  output;
		}

		bool isExonicSplicing() const
		{
			return ccds_overlap.length()>0;
		}
	};
	QList<GapInfo> gaps_;
	Ui::GapDialog* ui;
};

#endif // GAPDIALOG_H
