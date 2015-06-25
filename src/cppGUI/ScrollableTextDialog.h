#ifndef SCROLLABLETEXTDIALOG_H
#define SCROLLABLETEXTDIALOG_H

#include "cppGUI_global.h"
#include <QDialog>

namespace Ui {
class ScrollableTextDialog;
}

class CPPGUISHARED_EXPORT ScrollableTextDialog
	: public QDialog
{
	Q_OBJECT

public:
	ScrollableTextDialog(QWidget* parent = 0);
	~ScrollableTextDialog();

	void setText(QString lines);

private:
	Ui::ScrollableTextDialog* ui_;
};

#endif // SCROLLABLETEXTDIALOG_H
