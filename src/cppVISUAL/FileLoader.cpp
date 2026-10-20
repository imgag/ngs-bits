#include "FileLoader.h"
#include "IgvTrack.h"
#include "SegTrack.h"
#include "VersatileFile.h"
#include <cmath>
#include "BedTrack.h"
#include "VcfTrack.h"
#include "SvTrack.h"
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
	if (filename.endsWith(".bedpe", Qt::CaseInsensitive) || filename.endsWith(".bedpe.gz", Qt::CaseInsensitive))
	{
		output << SvTrack::createTrack(parent, file_path, filename);
		return output;
	}
	if (filename.endsWith(".vcf", Qt::CaseInsensitive) || filename.endsWith(".vcf.gz", Qt::CaseInsensitive))
	{
		output << VcfTrack::createTrack(parent, file_path, filename);
		return output;
	}
	if (filename.endsWith(".seg", Qt::CaseInsensitive))
	{
		output << SegTrack::createTrack(parent, file_path);
		return output;
	}
	if (filename.endsWith(".bed", Qt::CaseInsensitive) || filename.endsWith(".bed.gz", Qt::CaseInsensitive))
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
    QSharedPointer<BedFile> bedfile = QSharedPointer<BedFile>::create();
    bedfile->load(file_path);
    bedfile->sort();
    return bedfile;
}

QSharedPointer<IgvFile> FileLoader::loadIgvFile(QString file_path)
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

QSharedPointer<SegFile> FileLoader::loadSegFile(QString file_path)
{
    VersatileFile file(file_path);
    file.open(QFile::ReadOnly | QIODevice::Text);
    QVector<SegFileLine> lines;
    QByteArrayList headers;
    QByteArray annotation_name = "Annotation";
    bool found_header = false;
    int line_number = 0;
    while (!file.atEnd())
    {
        QByteArray line = file.readLine(true);
        ++line_number;
        if (line.isEmpty()) continue;
        if (line.startsWith('#') || line.startsWith("track "))
        {
            headers.append(line);
            continue;
        }
        const QByteArrayList columns = line.split('\t');
        if (columns.size() != 6) THROW(FileParseException, "Expected six SEG columns at line " + QString::number(line_number));
        if (!found_header)
        {
            annotation_name = columns[4];
            found_header = true;
            continue;
        }
        bool ok_start = false, ok_end = false, ok_value = false;
        const int start = columns[2].toInt(&ok_start);
        const int end = columns[3].toInt(&ok_end);
        const double value = columns[5].toDouble(&ok_value);
        const Chromosome chr(columns[1]);
        if (!chr.isValid() || !ok_start || !ok_end || start < 1 || end < start || !ok_value || !std::isfinite(value))
        {
            THROW(FileParseException, "Invalid SEG region or value at line " + QString::number(line_number));
        }
        lines.append(SegFileLine(chr, start, end, columns[4], value));
    }
    if (!found_header) THROW(FileParseException, "Missing SEG column header in " + file_path);
    return QSharedPointer<SegFile>::create(std::move(lines), std::move(headers), annotation_name);
}

QSharedPointer<BamReader> FileLoader::loadBamFile(QString file_path)
{
    QSharedPointer<BamReader> reader = QSharedPointer<BamReader>::create(file_path);
    return reader;
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
