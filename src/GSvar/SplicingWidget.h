#ifndef SPLICINGWIDGET_H
#define SPLICINGWIDGET_H

#include <QWidget>
#include <QMap>
#include <QTableWidget>

namespace Ui {
class SplicingWidget;
}

class SplicingWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SplicingWidget(const QString& splicing_file, QWidget *parent = 0);
	~SplicingWidget();

private slots:
	void applyFilter();
	void copyToClipboard();
	void OpenInIGV(QTableWidgetItem* item);

private:
	void loadSplicingFile(const QString& file_path);
	Ui::SplicingWidget *ui_;

	QMap<QByteArray,int> column_indices_;
};

#endif // SPLICINGWIDGET_H
