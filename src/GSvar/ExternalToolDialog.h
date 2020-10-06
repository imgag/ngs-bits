#ifndef EXTERNALTOOLDIALOG_H
#define EXTERNALTOOLDIALOG_H

#include <QDialog>
#include "ui_ExternalToolDialog.h"

class ExternalToolDialog
		: public QDialog
{
	Q_OBJECT

public:
	ExternalToolDialog(QString tool_name, QString mode, QWidget* parent = 0);

private slots:
	void browse();

protected:
	enum FileType
	{
		BAM,
		VCF,
		GSVAR,
		BED
	};

	//Returns a file name from the open file dialog or from NGSD via the processed sample name
	QString getFileName(FileType type, bool ngsd_instead_of_filesystem);

private:
	Ui::ExternalToolDialog ui_;
	QString tool_name_;
	QString mode_;

};

#endif // EXTERNALTOOLDIALOG_H
