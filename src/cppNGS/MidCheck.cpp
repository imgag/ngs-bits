#include "MidCheck.h"
#include "Exceptions.h"
#include "Helper.h"

QPair<int, int> MidCheck::parseRecipe(QString recipe)
{
	QStringList parts = recipe.split("+");
	if (parts.size()<3 || parts.size()>4) THROW(ArgumentException, "Invalid recipe '" + recipe + "' of run!");

	parts.removeFirst();
	parts.removeLast();

	int index1 = Helper::toInt(parts[0], "Index read 1");
	int index2 = parts.size()==1 ? 0 : Helper::toInt(parts[1], "Index read 2");

	return qMakePair(index1, index2);
}

QList<MidClash> MidCheck::check(QList<SampleMids> mids, int index1_length, int index2_length)
{
	//trim MIDs to maximum usable length
	for(int i=0; i<mids.count(); ++i)
	{
		SampleMids& s = mids[i];
		s.mid1_seq = s.mid1_seq.left(index1_length);
		s.mid2_seq = s.mid2_seq.left(index2_length);
	}

	QList<MidClash> output;
	for(int i=0; i<mids.count(); ++i)
	{
		for(int j=i+1; j<mids.count(); ++j)
		{
			//check lane
			TODO

			//compare
			int dist1 = Helper::levenshtein(samples[i].mid1, samples[j].mid1);
			int dist2 = -1;
			if (samples[i].mid2!="" && samples[j].mid2!="")
			{
				dist2 = Helper::levenshtein(samples[i].mid2, samples[j].mid2);
			}

			if (dist1==0 && dist2<1)
			{
				//text output
				QString mid = samples[i].mid1;
				if (dist2>=0) mid += "+" + samples[i].mid2;
				QString n1 = samples[i].name;
				QString n2 = samples[j].name;
				output << "clash " + n1 + " <=> " + n2 + " (" + mid + ")";

				//gui output
				highlightItem(i, 0, n2 + " (" + mid + ")");
				highlightItem(j, 0, n1 + " (" + mid + ")");
			}
		}
	}

	return output;
}
