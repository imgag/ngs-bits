#ifndef TRIODIALOG_H
#define TRIODIALOG_H

#include <QDialog>
#include "ui_TrioDialog.h"
#include "NGSD.h"

///Dialog for selecting a trio.
class TrioDialog
		: public QDialog
{
	Q_OBJECT

public:
	///Constructor
	explicit TrioDialog(QWidget* parent = 0);

	///Returns the processed sample list (child, father, mother)
	QStringList samples();

private slots:
	void on_add_samples_clicked(bool);
	void updateStartButton();

private:
	Ui::TrioDialog ui_;
	NGSD db_;
	struct SampleInfo
	{
		QString name;
		QString system;
		QString status;
		QString quality;
	};
	QList<SampleInfo> samples_;

	bool addSample(QString status);
	void clearSampleData();
	void updateSampleTable();
};

#endif // TRIODIALOG_H
