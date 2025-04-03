#include "MidCheck.h"
#include "Exceptions.h"
#include "Helper.h"

QPair<int, int> MidCheck::lengthFromRecipe(QString recipe)
{
	QStringList parts = recipe.split("+");
	if (parts.size()<3 || parts.size()>4) THROW(ArgumentException, "Invalid recipe '" + recipe + "' of run!");

	parts.removeFirst();
	parts.removeLast();

	int index1 = Helper::toInt(parts[0], "Index read 1");
	int index2 = parts.size()==1 ? 0 : Helper::toInt(parts[1], "Index read 2");

	return qMakePair(index1, index2);
}

QPair<int, int> MidCheck::lengthFromSamples(const QList<SampleMids>& mids)
{
	int index1 = 99;
	int index2 = 99;

	foreach(const SampleMids& mid, mids)
	{
        index1 = std::min(SIZE_TO_INT(index1), SIZE_TO_INT(mid.mid1_seq.length()));
        index2 = std::min(SIZE_TO_INT(index2), SIZE_TO_INT(mid.mid2_seq.length()));
	}

	return qMakePair(index1, index2);
}

QList<MidClash> MidCheck::check(QList<SampleMids> mids, int index1_length, int index2_length, QStringList& messages)
{
	//clear messages
	messages.clear();

	//trim MIDs to usable length
	for(int i=0; i<mids.count(); ++i)
	{
		SampleMids& s = mids[i];
		s.mid1_seq = s.mid1_seq.left(index1_length);
		s.mid2_seq = s.mid2_seq.left(index2_length);

		if (s.mid1_seq.size() < index1_length) messages << "Warning: MID 1 of sample " + s.name + " is short than used index length!";
		if (s.mid2_seq.size() < index2_length) messages << "Warning: MID 2 of sample " + s.name + " is short than used index length!";
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
		//qDebug() << "processing lane:" << QString::number(lane);

		for(int i=0; i<mids.count(); ++i)
		{
			if (!mids[i].lanes.contains(lane)) continue;

			for(int j=i+1; j<mids.count(); ++j)
			{
				if (!mids[j].lanes.contains(lane)) continue;

				//compare
				int dist1 = Helper::levenshtein(mids[i].mid1_seq, mids[j].mid1_seq);
				int dist2 = -1;
				if (mids[i].mid2_seq!="" && mids[j].mid2_seq!="")
				{
					dist2 = Helper::levenshtein(mids[i].mid2_seq, mids[j].mid2_seq);
				}

				if (dist1==0 && dist2<=0)
				{
					QString sequence = mids[i].mid1_seq + (dist2==-1 ? "" : "+" + mids[i].mid2_seq);
					messages << "Error: MID clash between samples " + mids[i].name + " and " + mids[j].name + " on lane " + QString::number(lane) + ". Common MID sequence is " + sequence + ".";

					output << MidClash {i, j};
				}
			}
		}
	}

	messages << (output.isEmpty() ? "No" : "Error: " + QString::number(output.count())) + " MID clashes found when using " + QString::number(index1_length) + "/" + QString::number(index2_length) + " index bases";

	return output;
}

QString SampleMids::lanesAsString() const
{
	QStringList output;

	foreach(int lane, lanes)
	{
		output << QString::number(lane);
	}

	std::sort(output.begin(), output.end());

	return output.join(",");
}
