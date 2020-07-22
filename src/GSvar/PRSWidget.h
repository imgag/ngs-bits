#ifndef PRSVIEW_H
#define PRSVIEW_H

#include <QTableWidgetItem>
#include <QWidget>

namespace Ui {
class PRSView;
}

class PRSWidget : public QWidget
{
	Q_OBJECT

public:
	explicit PRSWidget(QString filename, QWidget *parent = 0);
	~PRSWidget();

protected slots:
	void showContextMenu(QPoint pos);


private:
	void loadPrsData();
	Ui::PRSView *ui_;
	QString filename_;
	QByteArrayList column_header_;
};

#endif // PRSVIEW_H
