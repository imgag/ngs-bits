#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "ToolBase.h"
#include <zlib.h>
#include <QFileInfo>
#include "Helper.h"
#include "VcfFile.h"
#include "VariantList.h"
#include <QMutex>
#include "BigWigReader.h"

ChunkProcessor::ChunkProcessor(AnalysisJob &job, const QByteArray& name, const QByteArray& desc, const QByteArray& bw_filepath)

	:QRunnable()
	, terminate_(false)
	, job_(job)
	, name_(name)
	, desc_(desc)
	, bw_filepath_(bw_filepath)
	, bw_reader_(bw_filepath)
{
}

// single chunks are processed
void ChunkProcessor::run()
{
	job_.error_message.clear();
	job_.current_chunk_processed.clear();

	// read vcf file
	foreach(QByteArray line, job_.current_chunk)
	{
		while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);

		//skip empty lines
		if (line.trimmed().isEmpty()) continue;

		//write out headers unchanged
		if (line.startsWith('#'))
		{
			//append header line for new annotation
			if (line.startsWith("#CHROM"))
			{
				job_.current_chunk_processed.append("##INFO=<ID=" + name_ + ",Number=1,Type=Float,Description=\"" + desc_ + "\">\n");
			}
			job_.current_chunk_processed.append(line + "\n");
			continue;
		}

		//split line and extract variant infos
		QList<QByteArray> parts = line.split('\t');
		if (parts.count()<VcfFile::MIN_COLS)
		{
			job_.error_message.append("FileParseException: VCF line with too few columns: " + line);
			job_.status = ERROR;
			return;
		}
		Chromosome chr = parts[0];
		bool ok = false;
		int start = parts[1].toInt(&ok);
		if (!ok)
		{
			job_.error_message.append("FileParseException: Could not convert VCF variant position '" + parts[1] + "' to integer!");
			job_.status = ERROR;
			return;
		}
		int end = start + parts[3].length(); //length of ref
		QByteArray ref = parts[3];
		QByteArray alt = parts[4];
		if (alt.contains(',')) // if alt contains a list of alternatives choose the first one.
		{
			alt = alt.split(',')[0];
		}
		//get annotation data
        QList<float> anno = getAnnotation(chr.str(), start, end, ref, alt);

        if (anno.length() == 0)
        {
            // if there is no annotation to add, append the line unchanged:
			job_.current_chunk_processed.append(line + "\n");
            continue;
        }

		// add INFO column annotation
		if(parts[7] == ".") parts[7].clear(); // remove '.' if column was empty before
		if(!parts[7].isEmpty()) parts[7].append(';');
		parts[7].append(name_ + "=" + QByteArray::number(anno[0]));

		job_.current_chunk_processed.append(parts.join('\t') + "\n");
	}

	// annotation of job finished, clear input to keep memory usage low
	job_.current_chunk.clear();
	job_.status=TO_BE_WRITTEN;
}

QList<float> ChunkProcessor::getAnnotation(const QByteArray& chr, int start, int end, const QString& ref, const QString& alt)
{
    int offset = -1; // offset is -1 as the vcf file uses genome coordinates from 1 - N but bw-files from 0 - N-1

	// insertions are not annotated:
    if (alt.length() > ref.length())
    {
        return QList<float>();
    }

	if (ref[0] == alt[0])
	{
		return interpretIntervals(bw_reader_.getOverlappingIntervals(chr, start+1, end, offset));
	}

	return interpretIntervals(bw_reader_.getOverlappingIntervals(chr, start, end, offset));
}

QList<float> ChunkProcessor::interpretIntervals(const QList<OverlappingInterval>& intervals)
{
    if (intervals.length() == 0)
    {
        return QList<float>();
    }
    else if (intervals.length() == 1)
    {
        return QList<float>{intervals[0].value};
    }
    else
    {
        float max = std::numeric_limits<float>::lowest();
		foreach (const OverlappingInterval& i, intervals)
        {
            if (i.value > max)
            {
                max = i.value;
            }
        }
        return QList<float>{max};
    }
}
