#ifndef CHUNKPROCESSOR_H
#define CHUNKPROCESSOR_H

#include <QRunnable>
#include <QByteArray>
#include <QByteArrayList>
#include <QTextStream>
#include <iostream>
#include <QBuffer>
#include <QVector>
#include <QMutex>
#include "ChromosomalIndex.h"
#include "Auxilary.h"
#include "Exceptions.h"
#include "TabixIndexedFile.h"
#include "Helper.h"
#include "BedFile.h"

class ChunkProcessor
		:public QRunnable
{
public:
	ChunkProcessor(AnalysisJob &job_, QByteArray name_, const BedFile& bed_file_, const ChromosomalIndex<BedFile>& bed_index_, QByteArray bed_file_path_, QByteArray sep_);
	void run();

	void terminate()
	{
		terminate_ = true;
	}

private:
	bool terminate_;
	AnalysisJob& job;
	QByteArray name;
	const BedFile& bed_file;
	const ChromosomalIndex<BedFile>& bed_index;
	QByteArray bed_file_path;
	QByteArray sep;
};

#endif // CHUNKPROCESSOR_H
