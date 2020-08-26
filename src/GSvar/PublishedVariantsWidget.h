#ifndef PUBLISHEDVARIANTSWIDGET_H
#define PUBLISHEDVARIANTSWIDGET_H

#include <QWidget>

namespace Ui {
class PublishedVariantsWidget;
}

class PublishedVariantsWidget
	: public QWidget
{
	Q_OBJECT

public:
	PublishedVariantsWidget(QWidget* parent = 0);
	~PublishedVariantsWidget();


private slots:
	void updateTable();

private:
	Ui::PublishedVariantsWidget* ui_;
};

#endif // PUBLISHEDVARIANTSWIDGET_H
