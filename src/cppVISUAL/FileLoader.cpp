#include "FileLoader.h"
#include "IgvTrack.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include <QApplication>
#include <QMessageBox>
#include "Exceptions.h"

BedFile FileLoader::referenceNRegions(const FastaFileIndex& genome, const std::function<bool(int, int)>& progress)
{
	constexpr int chunk_size = 1024 * 1024;
	int total = 0;
	for (const Chromosome& chr : genome.chromosomes()) total += (genome.lengthOf(chr) + chunk_size - 1) / chunk_size;
	int completed = 0;
	BedFile regions;
	if (progress && !progress(0, total)) return regions;
	for (const Chromosome& chr : genome.chromosomes())
	{
		const int length = genome.lengthOf(chr);
		int n_start = 0;
		for (int start = 1; start <= length; start += chunk_size)
		{
			const Sequence sequence = genome.seq(chr, start, std::min(chunk_size, length - start + 1), false);
			for (int i = 0; i < sequence.size(); ++i)
			{
				if (sequence[i] == 'N' || sequence[i] == 'n')
				{
					if (n_start == 0) n_start = start + i;
				}
				else if (n_start > 0)
				{
					regions.append(BedLine(chr, n_start, start + i - 1));
					n_start = 0;
				}
			}
			if (progress && !progress(++completed, total)) return BedFile();
		}
		if (n_start > 0) regions.append(BedLine(chr, n_start, length));
	}
	regions.sort();
	return regions;
}

QVector<TrackWidget*> FileLoader::loadTracks(QString file_path, QWidget* parent)
{
	//get file name - for URLs removes paramters
	QString filename = TrackWidget::getDisplayNameFromFilePath(file_path);

	QVector<TrackWidget*> output;
	if (filename.endsWith(".bed"))
	{
		output << BedTrack::createTrack(parent, file_path, filename);
		return output;
	}
	if (filename.endsWith(".bam") || filename.endsWith(".cram"))
	{
		output << BamCoverageTrack::createTrack(parent, file_path, filename + " coverage");
		output << BamAlignmentTrack::createTrack(parent, file_path, filename);
		return output;
	}
	if (filename.endsWith(".igv"))
	{
		output << IgvTrack::createTrack(parent, file_path, filename);
		return output;
	}

	THROW(FileAccessException, "Unsupported file type for file: " + filename);
}

QSharedPointer<BedFile> FileLoader::loadBedFile(QString file_path)
{
	try
	{
		QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
		bedfile->load(file_path);
		bedfile->sort();
		return bedfile;
	}
	catch (const Exception& e)
	{
		QMessageBox::critical(QApplication::activeWindow(), "Error", e.message());
		return nullptr;
	}
}

QSharedPointer<IgvFile> FileLoader::loadIgvFile(QString file_path)
{
	try
	{
		QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
		bedfile->load(file_path);

		if (!isValidIgvFile(bedfile))
		{
			QMessageBox::critical(QApplication::activeWindow(), "Error", file_path + " is not a valid IGV file (header does not contain 5 columns)");
			return nullptr;
		}

		QVector<IgvFileLine> lines;
		lines.reserve(bedfile->count());
		for (int i = 0; i < bedfile->count(); ++i)
		{
			const BedLine& line = (*bedfile)[i];
			if (line.annotations().count() < 2) continue;
			bool ok = false;
			double value = line.annotations()[1].toDouble(&ok);
			if (!ok) continue;
			lines.append(IgvFileLine(line.chr(), line.start(), line.end(), value));
		}
		return QSharedPointer<IgvFile>::create(std::move(lines), bedfile->headers());
	}
	catch (const Exception& e)
	{
		QMessageBox::critical(QApplication::activeWindow(), "Error", e.message());
		return nullptr;
	}
}

QSharedPointer<BamReader> FileLoader::loadBamFile(QString file_path)
{
	try
	{
		QSharedPointer<BamReader> reader = QSharedPointer<BamReader>::create(file_path);
		return reader;
	}
	catch (const Exception& e)
	{
		QMessageBox::critical(QApplication::activeWindow(), "Error", e.message());
		return nullptr;
	}
}

bool FileLoader::isValidIgvFile(QSharedPointer<BedFile> bed_file)
{
	if (!bed_file) return false; // redundant check but avoids accidental crashes

	bool is_valid = true;

	foreach (const QByteArray& header, bed_file->headers())
	{
		if (header.startsWith("#")) continue;
		QList<QByteArray> columns = header.split('\t');

		if (columns.count() < 5) is_valid = false;
	}

	return is_valid & !bed_file->headers().empty();
}
