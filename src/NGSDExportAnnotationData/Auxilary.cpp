#include "Auxilary.h"

QByteArray getTimeString(double milliseconds)
{
	//calculate minutes and seconds
	double s = milliseconds/1000.0;
	double m = floor(s/60.0);
	s -= 60.0 * m;
	double h = floor(m/60.0);
	m -= 60.0 * h;

	//create strings
	QByteArray sec = QByteArray::number(s, 'f', 3) + "s";
	QByteArray min = m==0.0 ? "" : QByteArray::number(m, 'f', 0) + "m ";
	QByteArray hours = h==0.0 ? "" : QByteArray::number(h, 'f', 0) + "h ";

	return hours + min + sec;
}
