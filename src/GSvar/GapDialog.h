#ifndef GAPDIALOG_H
#define GAPDIALOG_H

#include "ui_GapDialog.h"
#include <QDialog>
#include <QTableWidgetItem>
#include "BedFile.h"
#include "GeneSet.h"
#include "DelayedInitializationTimer.h"
#include "NGSD.h"

class GapDialog
	: public QDialog
{
	Q_OBJECT

public:
	GapDialog(QWidget* parent, QString ps, QString bam_file, QString lowcov_file, const BedFile& roi, const GeneSet& genes);

signals:
	void openRegionInIGV(QString region);

private slots:
	void delayedInitialization();
	QStringList calculteGapsAndInitGUI();
	void gapDoubleClicked(QTableWidgetItem* item);
	void updateFilters();
	void updateNGSDColumn();
	void gapsContextMenu(QPoint pos);

private:
	Ui::GapDialog ui_;
	DelayedInitializationTimer init_timer_;
	NGSD db_;
	QString ps_;
	QString bam_;
	QString lowcov_file_;
	const BedFile& roi_;
	const GeneSet& genes_;
	int ngsd_col_;
	struct GapInfo
	{
		BedLine region;
		double avg_depth;
		GeneSet genes;
		BedLine coding_overlap;
		QString preferred_transcript;

		bool isExonicSplicing() const
		{
			return coding_overlap.length()>0;
		}
	};
	QList<GapInfo> gaps_;

	static void highlightItem(QTableWidgetItem* item);
};

#endif // GAPDIALOG_H
