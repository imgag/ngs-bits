#ifndef MIDCHECK_H
#define MIDCHECK_H

#include "cppNGS_global.h"

#include <QPair>
#include <QString>
#include <QList>
#include <QSet>

//Datastructure for sample MIDs
struct SampleMids
{
	QString name;

	QSet<int> lanes;

	QString mid1_name;
	QString mid1_seq;
	QString mid2_name;
	QString mid2_seq;
};

//Datastructure for MID clash
struct MidClash
{
	int s1_index;
	int s2_index;

	QString message;
};

//MID clash check
class CPPNGSSHARED_EXPORT MidCheck
{
public:
	static QPair<int, int> parseRecipe(QString recipe);
	static QList<MidClash> check(QList<SampleMids> mids, int index1_length, int index2_length);

protected:
	MidCheck() = delete;
};

#endif // MIDCHECK_H
