#ifndef NGSHELPER_H
#define NGSHELPER_H

#include "cppNGS_global.h"
#include "BamReader.h"
#include "FilterCascade.h"
#include "GenomeBuild.h"
#include "Transcript.h"

//Helper datastructure for gene impringing info.
struct ImprintingInfo
{
	QByteArray expressed_allele;
	QByteArray status;
};

//Target region information.
struct TargetRegionInfo
{
	QString name;
	BedFile regions;
	GeneSet genes;

	//Returns if a target region is set
	bool isValid() const
	{
		return !regions.isEmpty();
	}

	//Clears all data
	void clear()
	{
		name.clear();
		regions.clear();
		genes.clear();
	}

};

//Settings for Gff parser
struct GffSettings
{
	bool skip_not_gencode_basic = true; //skip transcripts that are not flagged as "GENCODE basic"
	bool print_to_stdout = true; //print summary to stdout
};

//Output of Ensembl GFF file parser
struct GffData
{
	//Transcripts
	TranscriptList transcripts;

	//Map from ENST to ENSG.
	QHash<QByteArray, QByteArray> enst2ensg;

	//Map from ENSG to gene symbol
	QHash<QByteArray, QByteArray> ensg2symbol;

	//Set of ENST identifiers that are flagged as GENCODE basic
	QSet<QByteArray> gencode_basic;
};

//Contains information about the GSvarServer (used in client-server mode)
struct CPPNGSSHARED_EXPORT ServerInfo
{
	QString version; // server version
	QString api_version; // server API version
	QDateTime server_start_time; // date and time when the server was started

	bool isEmpty()
	{
		return version.isEmpty() && api_version.isEmpty() && server_start_time.isNull();
	}
};

// Information about the current version of the desktop clinet application (e.g. needed to inform the user
// about updates)
struct CPPNGSSHARED_EXPORT ClientInfo
{
	QString version;
	QString message;
	QDateTime date;

	ClientInfo()
		: version()
		, message()
		, date()
	{
	}

	ClientInfo(const QString& version_, const QString& message_, const QDateTime date_ = QDateTime::currentDateTime())
		: version(version_)
		, message(message_)
		, date(date_)
	{
	}

	QList<int> getVersionParts()
	{
		QList<int> output;
		QStringList version_present_parts_start = version.split("_");
		if (version_present_parts_start.size()>1)
		{
			output.append(version_present_parts_start[0].toInt());
			QStringList version_present_parts_end = version_present_parts_start[1].split("-");
			if (version_present_parts_end.size()>1)
			{
				output.append(version_present_parts_end[0].toInt());
				output.append(version_present_parts_end[1].toInt());
			}
		}
		if (output.size()<3) output = QList<int> {0, 0, 0};
		return output;
	}

	bool isOlderThan(ClientInfo info_provided)
	{
		QList<int> present = getVersionParts();
		QList<int> provided = info_provided.getVersionParts();

		bool is_older = false;

		if (present[0]<provided[0]) is_older = true;
		if (present[0]>provided[0]) return false;
		if (!is_older && (present[1]<provided[1])) is_older = true;
		if (present[1]>provided[1]) return false;
		if (!is_older && (present[2]<provided[2])) is_older = true;
		return is_older;
	}

	bool isOlderThan(QString version)
	{
		return isOlderThan(ClientInfo(version,""));
	}

	QJsonObject toJsonObject()
	{
		QJsonObject out;
		out.insert("version", version);
		out.insert("message", message);
		out.insert("date", date.toSecsSinceEpoch());
		return out;
	}

	bool isEmpty()
	{		
		return version.isEmpty() && message.isEmpty() && date.isNull();
	}
};

// Stores a notification displayed to the user of the client application (e.g. downtimes, maintenance, updates)
struct CPPNGSSHARED_EXPORT UserNotification
{
	QString id;
	QString message;

	UserNotification()
		: id()
		, message()
	{
	}

	UserNotification(QString id_, QString message_)
		: id(id_)
		, message(message_)
	{
	}

	bool isEmpty()
	{
		return id.isEmpty() && message.isEmpty();
	}
};

///Helper class for NGS-specific stuff.
class CPPNGSSHARED_EXPORT NGSHelper
{
public:
	///Returns known SNPs and indels from gnomAD (AF>=1%, AN>=5000).
	static VcfFile getKnownVariants(GenomeBuild build, bool only_snvs, const BedFile& roi, double min_af=0.0, double max_af=1.0);
	static VcfFile getKnownVariants(GenomeBuild build, bool only_snvs, double min_af=0.0, double max_af=1.0);

	///Soft-clip alignment from the beginning or end (positions are 1-based)
	static void softClipAlignment(BamAlignment& al, int start_ref_pos, int end_ref_pos);

	///Create sample overview file
	static void createSampleOverview(QStringList in, QString out, int indel_window=100, bool cols_auto=true, QStringList cols = QStringList());

	///Translates a codon to the 1-letter amino acid code
	static char translateCodon(const QByteArray& codon, bool use_mito_table=false);
	static QByteArray translateCodonThreeLetterCode(const QByteArray& codon, bool use_mito_table=false);

	///Converts a 1-letter amino acid code to a 3-letter amino acid code
	static QByteArray threeLetterCode(char aa_one_letter_code);

	///Converts a 3-letter amino acid code to a 1-letter amino acid code
	static char oneLetterCode(const QByteArray& aa_tree_letter_code);

	///Returns the pseudoautomal regions on gnosomes.
	static const BedFile& pseudoAutosomalRegion(GenomeBuild build);

	///Returns the cytogenetic band for to chromosomal position
	static QByteArray cytoBand(GenomeBuild build, Chromosome chr, int pos);
	///Returns the chromosomal range of a cytoband or cytoband range.
	static BedLine cytoBandToRange(GenomeBuild build, QByteArray cytoband);

	///Returns a map if imprinted genes and inherited allele.
	static const QMap<QByteArray, ImprintingInfo>& imprintingGenes();

	///Parses a chromosomal region from the given text. Throws an error, if the region is not valid.
	static void parseRegion(const QString& text, Chromosome& chr, int& start, int& end);

	///Returns Bed File with coordinates of centromeres.
	static const BedFile& centromeres(GenomeBuild build);

	///Returns Bed file with coordinates of telomeres.
	static const BedFile& telomeres(GenomeBuild build);

	///Converts the 3letter ancestry code to a human-readable text, see http://m.ensembl.org/Help/Faq?id=532
	static QString populationCodeToHumanReadable(QString code);

	///Returns transcripts with features from a Ensembl GFF file, transcript_gene_relation (ENST>ENSG) and gene_name_relation (ENSG>gene symbol).
	static GffData loadGffFile(QString filename, GffSettings settings);

	///Returns if the application is running in client-server mode (mainly used for GSvar).
	static bool isClientServerMode();
	///Checks if the application is running on the server or on a client machine
	static bool isRunningOnServer();
	///Checks if a given local file or URL is a BAM file
	static bool isBamFile(QString filename);
	///Removes a secure token from the URL that is given to IGV
	static QString stripSecureToken(QString url);

	///Requests information about GSvarServer
	static ServerInfo getServerInfo();

	///Requests the client version number from the server to inform about updates (if available)
	static ClientInfo getClientInfo();

	///Requests the user notification from the server
	static UserNotification getUserNotification();

	///Returns the server API version. Used to check that the server and the client have the same version.
	static QString serverApiVersion();

	///Returns the URL used for sending requests to the GSvar server (use only when in client-server mode)
	static QString serverApiUrl();

	///Returns a map with matching Ensembl, RefSeq and CCDS transcript identifiers (without version numbers).
	static const QMap<QByteArray, QByteArrayList>& transcriptMatches(GenomeBuild build);

private:
	///Constructor declared away
	NGSHelper() = delete;
};

#endif // NGSHELPER_H
