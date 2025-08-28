#include "TabBaseClass.h"

TabBaseClass::TabBaseClass(QWidget *parent) : QWidget(parent)
{
}

bool TabBaseClass::isBusy() const
{
	return is_busy_;
}
