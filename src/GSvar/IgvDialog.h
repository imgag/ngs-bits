#ifndef IGVDIALOG_H
#define IGVDIALOG_H

#include <QDialog>

namespace Ui {
class IgvDialog;
}

class IgvDialog : public QDialog
{
	Q_OBJECT

public:
	explicit IgvDialog(QWidget *parent = 0);
	~IgvDialog();

	void addFile(QString label, QString filename, bool checked);
	void addSeparator();

	bool skipForSession() const;
	QStringList filesToLoad();

protected slots:
	void on_skip_session_clicked();

private:
	Ui::IgvDialog *ui;
	bool skip_session_;
};

#endif // IGVDIALOG_H
