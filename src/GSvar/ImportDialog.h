#ifndef IMPORTDIALOG_H
#define IMPORTDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QTableWidgetItem>
#include "ui_ImportDialog.h"
#include "FastaFileIndex.h"
#include "NGSD.h"

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
		RUNS,
		PROCESSED_SAMPLES,
		MIDS,
		STUDY_SAMPLE,
		SAMPLE_RELATIONS,
		SAMPLE_HPOS
	};

	ImportDialog(QWidget* parent,  Type type);

private slots:
	void pasteTable();
	void import();
	void variantImportFailed();

private:
	Ui::ImportDialog ui_;
	Type type_;
	NGSD db_;
	FastaFileIndex ref_genome_idx_;
	QString db_table_;
	QStringList db_fields_;
	int special_fields_; //number of special fields after regular DB fields, which are entered by the user but there is a special handling when importing.
	QStringList db_extra_fields_; //extra field names for which the value is generated automatically by extraValues()

	void setupGUI();
	void pasteRow(int row_index, QString line);
	void keyPressEvent(QKeyEvent* e) override;
	void fixValue(QString value, const TableFieldInfo& field_info, QString& actual, QString& validation_error);
	bool addItem(int r, int c, const QString& value, const QString& actual, const QString& validation_error, const QString& notice);
	void checkNumberOfParts(const QStringList& parts);
	QString insertQuery();
	void addRow(SqlQuery& query, int r);
	QStringList extraValues(int r);

};

#endif // IMPORTDIALOG_H
