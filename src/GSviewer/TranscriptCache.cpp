#include "TranscriptCache.h"
#include "GffData.h"
#include "Helper.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
//Bump when the format, transcript enums, or GFF loading semantics change.
constexpr int CACHE_VERSION = 1;
const QByteArray METADATA_PREFIX = "#GSviewer transcript cache\t";
const QByteArray COLUMNS = "#gene\tgene_id\thgnc_id\ttranscript\tversion\tccds\tsource\tchromosome\tstrand\tbiotype\tcoding_start\tcoding_end\tflags\texons\n";

QJsonObject sourceMetadata(const QFileInfo& source)
{
	return {{"version", CACHE_VERSION}, {"gff_file", source.canonicalFilePath()},
		{"modified", source.lastModified().toUTC().toString(Qt::ISODateWithMs)}, {"size", QString::number(source.size())}};
}

QByteArray encode(QByteArray value)
{
	value.replace("%", "%25");
	value.replace("\t", "%09");
	value.replace("\n", "%0A");
	value.replace("\r", "%0D");
	return value;
}

QByteArray decode(QByteArrayView value)
{
	if (!value.contains('%')) return value.toByteArray();
	QByteArray result;
	result.reserve(value.size());
	for (qsizetype i = 0; i < value.size(); ++i)
	{
		if (value[i] != '%')
		{
			result += value[i];
			continue;
		}
		const QByteArrayView escape = value.sliced(i, std::min(qsizetype(3), value.size() - i));
		if (escape == "%25") result += '%';
		else if (escape == "%09") result += '\t';
		else if (escape == "%0A") result += '\n';
		else if (escape == "%0D") result += '\r';
		else THROW(FileParseException, "Invalid escape in transcript cache.");
		i += 2;
	}
	return result;
}

int transcriptFlags(const Transcript& t)
{
	return int(t.isPreferredTranscript()) | (int(t.isGencodeBasicTranscript()) << 1) | (int(t.isGencodePrimaryTranscript()) << 2) | (int(t.isEnsemblCanonicalTranscript()) << 3) | (int(t.isManeSelectTranscript()) << 4) | (int(t.isManePlusClinicalTranscript()) << 5);
}

TranscriptList loadGff(const QString& filename)
{
	GffSettings settings;
	settings.print_to_stdout = false;
	const GffData data = GffData::load(filename, settings);
	TranscriptList transcripts;
	transcripts.reserve(data.transcripts.count());
	for (const Transcript& transcript : data.transcripts)
	{
		if (transcript.source() == Transcript::ENSEMBL) transcripts << transcript;
	}
	if (!transcripts.isSorted()) transcripts.sortByPosition();
	return transcripts;
}

bool loadCache(const QString& filename, const QJsonObject& expected_metadata, TranscriptList& transcripts)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly)) THROW(FileAccessException, file.errorString());
	const QByteArray header = file.readLine();
	if (!header.startsWith(METADATA_PREFIX)) return false;
	QJsonParseError error;
	const QJsonDocument document = QJsonDocument::fromJson(header.mid(METADATA_PREFIX.size()), &error);
	if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
	QJsonObject metadata = document.object();
	const QJsonValue count_value = metadata.take("transcripts");
	const int count = count_value.toInt(-1);
	if (metadata != expected_metadata || !count_value.isDouble() || count < 0 || count_value.toDouble() != count) return false;
	if (file.readLine() != COLUMNS || count > file.size() / 14) return false;
	transcripts.reserve(count);
	QList<QByteArrayView> columns;
	columns.resize(14);
	while (!file.atEnd())
	{
		QByteArray row = file.readLine();
		if (row.endsWith('\n')) row.chop(1);
		if (Helper::fastSplit(row, '\t', columns) != 14) THROW(FileParseException, "Invalid transcript cache column count.");
		auto number = [&](int column)
		{ return Helper::toInt(columns[column], "transcript cache column " + QString::number(column)); };
		Transcript t;
		t.setGene(decode(columns[0]));
		t.setGeneId(decode(columns[1]));
		t.setHgncId(decode(columns[2]));
		t.setName(decode(columns[3]));
		t.setVersion(number(4));
		t.setNameCcds(decode(columns[5]));
		if (number(6) != Transcript::ENSEMBL) THROW(FileParseException, "Invalid transcript cache source.");
		t.setSource(Transcript::ENSEMBL);
		const Chromosome chr(decode(columns[7]));
		const int strand = number(8);
		const int biotype = number(9);
		const int flags = number(12);
		if (!chr.isValid() || (strand != Transcript::PLUS && strand != Transcript::MINUS) || biotype < Transcript::IG_C_GENE || biotype > Transcript::PROTEIN_CODING_CDS_NOT_DEFINED || (flags & ~63))
		{
			THROW(FileParseException, "Invalid transcript cache metadata.");
		}
		t.setStrand(static_cast<Transcript::STRAND>(strand));
		t.setBiotype(static_cast<Transcript::BIOTYPE>(biotype));
		t.setPreferredTranscript(flags & 1);
		t.setGencodeBasicTranscript(flags & 2);
		t.setGencodePrimaryTranscript(flags & 4);
		t.setEnsemblCanonicalTranscript(flags & 8);
		t.setManeSelectTranscript(flags & 16);
		t.setManePlusClinicalTranscript(flags & 32);
		BedFile exons;
		QByteArrayView intervals = columns[13];
		while (!intervals.isEmpty())
		{
			const qsizetype comma = intervals.indexOf(',');
			const QByteArrayView interval = comma < 0 ? intervals : intervals.first(comma);
			const qsizetype dash = interval.indexOf('-');
			if (dash <= 0) THROW(FileParseException, "Invalid transcript cache exon interval.");
			const int start = Helper::toInt(interval.first(dash), "cached exon start");
			const int end = Helper::toInt(interval.sliced(dash + 1), "cached exon end");
			if (start < 1 || end < start) THROW(FileParseException, "Invalid transcript cache exon coordinates.");
			exons.append(BedLine(chr, start, end));
			if (comma < 0) break;
			intervals = intervals.sliced(comma + 1);
			if (intervals.isEmpty()) THROW(FileParseException, "Missing transcript cache exon interval.");
		}
		const int coding_start = number(10);
		const int coding_end = number(11);
		if (exons.isEmpty() || t.name().isEmpty() || ((coding_start == 0) != (coding_end == 0)))
		{
			THROW(FileParseException, "Incomplete transcript cache record.");
		}
		t.setRegions(exons, coding_start, coding_end);
		if (t.isCoding() && (std::min(coding_start, coding_end) < t.start() || std::max(coding_start, coding_end) > t.end()))
		{
			THROW(FileParseException, "Invalid transcript cache coding coordinates.");
		}
		transcripts << t;
		if (transcripts.count() > count) THROW(FileParseException, "Too many transcript cache records.");
	}
	if (file.error() != QFileDevice::NoError) THROW(FileAccessException, file.errorString());
	return transcripts.count() == count && transcripts.isSorted();
}

void storeCache(const QString& filename, QJsonObject metadata, const TranscriptList& transcripts)
{
	QSaveFile file(filename);
	if (!file.open(QIODevice::WriteOnly)) THROW(FileAccessException, file.errorString());
	auto write = [&](const QByteArray& text)
	{
		if (file.write(text) != text.size()) THROW(FileAccessException, file.errorString());
	};
	metadata.insert("transcripts", transcripts.count());
	write(METADATA_PREFIX + QJsonDocument(metadata).toJson(QJsonDocument::Compact) + '\n');
	write(COLUMNS);
	for (const Transcript& t : transcripts)
	{
		QByteArray row;
		row.reserve(256 + t.regions().count() * 20);
		for (const auto& field : {t.gene(), t.geneId(), t.hgncId(), t.name()}) row += encode(field) + '\t';
		row += QByteArray::number(t.version()) + '\t' + encode(t.nameCcds()) + '\t';
		row += QByteArray::number(int(t.source())) + '\t' + encode(t.chr().str()) + '\t';
		row += QByteArray::number(int(t.strand())) + '\t' + QByteArray::number(int(t.biotype())) + '\t';
		row += QByteArray::number(t.codingStart()) + '\t' + QByteArray::number(t.codingEnd()) + '\t';
		row += QByteArray::number(transcriptFlags(t)) + '\t';
		for (int i = 0; i < t.regions().count(); ++i)
		{
			if (i) row += ',';
			row += QByteArray::number(t.regions()[i].start()) + '-' + QByteArray::number(t.regions()[i].end());
		}
		write(row + '\n');
	}
	if (!file.commit()) THROW(FileAccessException, file.errorString());
}
}

TranscriptList TranscriptCache::load(const QString& gff_file, const QString& cache_directory)
{
	const QFileInfo source(gff_file);
	//Remote files and inaccessible local files use the existing GFF loader.
	if (!source.isFile()) return loadGff(gff_file);
	const QJsonObject metadata = sourceMetadata(source);
	QString directory = cache_directory;
	if (directory.isEmpty())
	{
		const QString app_data = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
		if (app_data.isEmpty()) return loadGff(gff_file);
		directory = QDir(app_data).filePath("transcript_cache");
	}
	//Hash only the path, not the GFF contents, to distinguish equally named source files.
	const QByteArray path_hash = QCryptographicHash::hash(source.canonicalFilePath().toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
	const QString cache_file = QDir(directory).filePath(source.fileName() + "." + path_hash + ".tsv");
	if (QFile::exists(cache_file))
	{
		try
		{
			TranscriptList transcripts;
			if (loadCache(cache_file, metadata, transcripts) && sourceMetadata(QFileInfo(gff_file)) == metadata)
			{
				qDebug() << "Loaded transcript cache:" << cache_file;
				return transcripts;
			}
		}
		catch (const Exception& e)
		{
			qWarning() << "Could not load transcript cache:" << cache_file << e.message();
		}
	}
	TranscriptList transcripts = loadGff(gff_file);
	try
	{
		//Do not associate data with stale metadata if the source changed while parsing.
		if (sourceMetadata(QFileInfo(gff_file)) == metadata)
		{
			if (!QDir().mkpath(directory)) THROW(FileAccessException, "Cannot create transcript cache directory: " + directory);
			storeCache(cache_file, metadata, transcripts);
			qDebug() << "Stored transcript cache:" << cache_file;
		}
	}
	catch (const Exception& e)
	{
		qWarning() << "Could not store transcript cache:" << cache_file << e.message();
	}
	return transcripts;
}
