#include "FileLoader.h"
#include "IgvTrack.h"
#include "BedTrack.h"
#include "BamAlignmentTrack.h"
#include "BamCoverageTrack.h"
#include <QApplication>
#include <QMessageBox>
#include "Exceptions.h"

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

QSharedPointer<BedFile> FileLoader::loadIgvFile(QString file_path)
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

		bedfile->sort();
		return bedfile;
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

//TODO: this should check more things, i.e. if the 5th column has numeric values
// if the 4th column is Features
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
