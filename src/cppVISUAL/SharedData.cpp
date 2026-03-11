#include "SharedData.h"
#include "Settings.h"

void SharedData::setTranscripts(const TranscriptList& transcipts)
{
	//copy Ensembl transcripts
	TranscriptList transcripts_ = instance()->transcripts_;
    for(const Transcript& t: transcipts)
    {
        if (t.source()!=Transcript::ENSEMBL) continue;
		instance()->transcripts_ << t;
    }

	//sort transcripts
	if (!instance()->transcripts_.isSorted())
	{
		instance()->transcripts_.sortByPosition();
	}

	//index transcripts
	instance()->transcripts_index_.createIndex();

	//emit signal
	instance()->transcriptsChanged();
}

void SharedData::setRegion(const Chromosome& chr, int start, int end)
{
	//extend region to minimal region size
	int size = end-start+1;
	if (size<instance()->settings_.min_window_size)
	{
		int missing = instance()->settings_.min_window_size-size;
		start -= missing/2;
		end += missing/2;
		if (missing%2!=0)
		{
			start -= 1;
			end += 1;
		}
		size = end-start+1;
	}

	//make sure the region is in chromosome boundaries
	if (start<1)
	{
		start = 1;
		end = start + size - 1;
	}
	int genome_end = SharedData::genome().lengthOf(chr);
	if (end>genome_end)
	{
		end = genome_end;
		start = end - size + 1;
		if (start<1) start = 1; //if size is bigger than chromosome, this can happen
	}

	//check new region is different from old
	BedLine new_reg = BedLine(chr, start, end);
	if (new_reg==instance()->region_) return;

	instance()->region_ = new_reg;
	instance()->regionChanged(); //emit signal
}

SharedData::SharedData(QObject* parent)
	: QObject(parent)
	, genome_index_(Settings::string("reference_genome", false))
	, transcripts_()
	, transcripts_index_(transcripts_)
	, settings_()
	, region_()
{
}
