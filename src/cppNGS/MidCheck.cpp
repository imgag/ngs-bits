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
	//trim MIDs to usable length
	for(int i=0; i<mids.count(); ++i)
	{
		SampleMids& s = mids[i];
		s.mid1_seq = s.mid1_seq.left(index1_length);
		s.mid2_seq = s.mid2_seq.left(index2_length);
	}

	//determine lanes
	QList<int> lanes;
	foreach(const SampleMids& mid, mids)
	{
		foreach(int lane, mid.lanes)
		{
			if (!lanes.contains(lane)) lanes << lane;
		}
	}
	std::sort(lanes.begin(), lanes.end());

	//find clashes
	QList<MidClash> output;
	foreach(int lane, lanes)
	{
		for(int i=0; i<mids.count(); ++i)
		{
			if (!mids[i].lanes.contains(lane)) continue;

			for(int j=i+1; j<mids.count(); ++j)
			{
				if (!mids[i].lanes.contains(lane)) continue;

				//compare
				int dist1 = Helper::levenshtein(mids[i].mid1_seq, mids[j].mid1_seq);
				int dist2 = -1;
				if (mids[i].mid2_seq!="" && mids[j].mid2_seq!="")
				{
					dist2 = Helper::levenshtein(mids[i].mid2_seq, mids[j].mid2_seq);
				}

				if (dist1==0 && dist2<=0)
				{
					MidClash clash;
					clash.s1_index = i;
					clash.s2_index = j;
					clash.message = "MID clash: " + mids[i].mid1_seq + (dist2==-1 ? "" : mids[i].mid2_seq);
					output << clash;
				}
			}
		}
	}

	return output;
}
