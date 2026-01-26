#ifndef TABBASECLASS_H
#define TABBASECLASS_H

#include <QWidget>

//Base class for widgets that are shown as a GSvar tab
class TabBaseClass
	: public QWidget
{
	Q_OBJECT
public:
	explicit TabBaseClass(QWidget *parent = nullptr);

	//Returns if the widget is busy (used to stop the user from closing the tab in that case).
	bool isBusy() const;

protected:
	bool is_busy_ = false;
};

#endif // TABBASECLASS_H
