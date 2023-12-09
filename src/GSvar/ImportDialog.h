#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include "ui_ImportDialog.h"
#include "FastaFileIndex.h"

class ImportDialog
	: public QDialog
{
Q_OBJECT

public:

	//Import type
	enum Type
	{
		VARIANTS,
		SAMPLES,
		PROCESSED_SAMPLES
	};

	ImportDialog(QWidget* parent,  Type type);

	//Function to open variants in GSvar tab
	void openVariants();

private:
	Ui::ImportDialog ui_;
	Type type_;
	FastaFileIndex ref_genome_idx_;

	void setupGUI();
	void handlePaste(QStringList lines);
	void keyPressEvent(QKeyEvent* e) override;
};

#endif // IMPORTDIALOG_H
