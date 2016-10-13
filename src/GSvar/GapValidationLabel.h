#ifndef GAPVALIDATIONLABEL_H
#define GAPVALIDATIONLABEL_H

#include <QWidget>

namespace Ui {
class GapValidationLabel;
}

class GapValidationLabel
	: public QWidget
{
	Q_OBJECT

public:
	explicit GapValidationLabel(QWidget *parent = 0);
	~GapValidationLabel();

	enum State
	{
		VALIDATION,
		NO_VALIDATION,
		CHECK
	};
	State state() const;
	void setState(State state);

protected slots:
	void contextMenu(QPoint p);

private:
	Ui::GapValidationLabel *ui;
	State state_;
};

#endif // GAPVALIDATIONLABEL_H
