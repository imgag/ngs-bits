#include "FileLoader.h"
#include "SharedData.h"
#include "Settings.h"

#include <QPainter>
#include <QFileInfo>
#include <QMessageBox>

void SharedData::setTranscripts(const TranscriptList& transcripts)
{
	SharedData* inst = instance();

	//copy Ensembl transcripts
	inst->transcripts_.clear();
	for(const Transcript& t: transcripts)
    {
        if (t.source()!=Transcript::ENSEMBL) continue;
		inst->transcripts_ << t;
    }

	//sort transcripts
	if (!inst->transcripts_.isSorted())
	{
		inst->transcripts_.sortByPosition();
	}

	//index transcripts
	inst->transcripts_index_.createIndex();

	emit inst->transcriptsChanged();
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
	emit instance()->regionChanged();
}

SharedData* SharedData::instance()
{
	static SharedData s;
	return &s;
}

SharedData::SharedData(QObject* parent)
	: QObject(parent)
	, genome_index_(Settings::string("reference_genome", false))
	, transcripts_()
	, transcripts_index_(transcripts_)
	, settings_()
	, region_()
	, char_size_(determineCharacterSize())
{
}

QSize SharedData::determineCharacterSize()
{
	QPainter painter;
	QFontMetrics fm(painter.font());

	int w = 0;
	int h = fm.height(); // font height already covers all characters
	for (QChar c : QString("ACGTN"))
	{
		w = std::max(w, fm.horizontalAdvance(c));
	}
	return QSize(w, h);
}

void SharedData::loadTrack(QString file_path)
{
	TrackList tracks = FileLoader::load(file_path);
	if (!tracks.empty()) emit instance()->tracksAdded(tracks);
}
