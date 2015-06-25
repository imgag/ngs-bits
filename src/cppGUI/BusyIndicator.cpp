#include "BusyIndicator.h"
#include <QMovie>

BusyIndicator::BusyIndicator(QWidget *parent)
	: QLabel(parent)
{
	movie_ = new QMovie("://Resources/busy.gif");
	movie_->setScaledSize(QSize(16, 16));
	setMovie(movie_);
	movie_->start();

	setToolTip("processing...");
}

BusyIndicator::~BusyIndicator()
{
	delete movie_;
}

