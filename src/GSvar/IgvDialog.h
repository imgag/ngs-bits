#ifndef IGVDIALOG_H
#define IGVDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>

namespace Ui {
class IgvDialog;
}

class IgvDialog : public QDialog
{
	Q_OBJECT

public:
	explicit IgvDialog(QWidget *parent = 0);
	~IgvDialog();

	void addFile(QString label, QString type, QString filename, bool checked);

	bool skipForSession() const;
	QStringList filesToLoad();

protected slots:
	void on_skip_session_clicked();
	void treeItemChanged(QTreeWidgetItem* item);

private:
	Ui::IgvDialog *ui;
	bool skip_session_;
};

#endif // IGVDIALOG_H
