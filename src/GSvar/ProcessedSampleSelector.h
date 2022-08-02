#ifndef PROCESSEDSAMPLESELECTOR_H
#define PROCESSEDSAMPLESELECTOR_H

#include <QDialog>
#include "ui_ProcessedSampleSelector.h"

#include "NGSD.h"
#include "DBSelector.h"

namespace Ui {
class ProcessedSampleSelector;
}

//Processed sample selection dialog.
class ProcessedSampleSelector
	: public QDialog
{
	Q_OBJECT

public:
	//Constructor
	ProcessedSampleSelector(QWidget* parent, bool include_merged_samples);
	//Set label text
	void setLabel(QString label);
	//Set processed sample selection text
	void setSelection(QString processed_sample);

	//Returns if a valid processed sample was selected
	bool isValidSelection() const;
	//Returns the processed sample database ID, or "" if the text it not a valid processed sample.
	QString processedSampleId() const;
	//Returns the processed sample name, or "" if the text it not a valid processed sample.
	QString processedSampleName() const;

	//Show multi/trio search checkbox
	void showSearchMulti(bool checked);
	//Returns if the search for multi/trio analysis is enabled
	bool searchMulti() const;


protected slots:
	void initAutoCompletion();
	void openById();

private:
	Ui::ProcessedSampleSelector ui_;
	mutable NGSD db_;
};

#endif // PROCESSEDSAMPLESELECTOR_H
