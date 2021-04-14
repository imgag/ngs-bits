#include "FileLocationProviderRemote.h"
#include "HttpRequestHandler.h"

FileLocationProviderRemote::FileLocationProviderRemote(const QString sample_id,const QString server_host, const int server_port)
	: sample_id_(sample_id)
	, server_host_(server_host)
	, server_port_(server_port)
{
}

bool FileLocationProviderRemote::isLocal() const
{
	return false;
}

FileLocation FileLocationProviderRemote::getAnalysisVcf() const
{
	return getOneFileLocationByType("analysisvcf", "");
}

FileLocation FileLocationProviderRemote::getAnalysisSvFile() const
{
	return getOneFileLocationByType("analysissv", "");
}

FileLocation FileLocationProviderRemote::getAnalysisCnvFile() const
{
	return getOneFileLocationByType("analysiscnv", "");
}

FileLocation FileLocationProviderRemote::getAnalysisMosaicCnvFile() const
{
	return getOneFileLocationByType("analysismosaiccnv", "");
}

FileLocation FileLocationProviderRemote::getAnalysisUpdFile() const
{
	return getOneFileLocationByType("analysisupd", "");
}

FileLocation FileLocationProviderRemote::getRepeatExpansionImage(QString locus) const
{
	return getOneFileLocationByType("repeatexpansionimage", locus);
}

FileLocationList FileLocationProviderRemote::getFileLocationsByType(QString type, bool return_if_missing) const
{
	FileLocationList output;
	if (sample_id_.isEmpty())
	{
		THROW(ArgumentException, "File name has not been specified")
		return output;
	}

	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	QString reply = HttpRequestHandler(HttpRequestHandler::NONE).get(
				"https://" + server_host_ + ":" + QString::number(server_port_)
				+ "/v1/file_location?ps=" + sample_id_ + "&type=" + type
				+ "&return_if_missing=" +(return_if_missing ? "1" : "0"), add_headers);

	QJsonDocument json_doc = QJsonDocument::fromJson(reply.toLatin1());
	QJsonArray file_list = json_doc.array();

	if (file_list.count() == 0)
	{
		THROW(Exception, "Could not find file info: " + type);
	}

	output = mapJsonArrayToFileLocationList(file_list, return_if_missing);
	return output;
}

FileLocation FileLocationProviderRemote::getOneFileLocationByType(QString type, QString locus) const
{
	FileLocation output;
	if (sample_id_.isEmpty())
	{
		THROW(ArgumentException, "File name has not been specified")
		return output;
	}

	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	QString reply = HttpRequestHandler(HttpRequestHandler::NONE).get(
				"https://" + server_host_ + ":" + QString::number(server_port_)
				+ "/v1/file_location?ps=" + sample_id_ + "&type=" + type
				+ (locus.isEmpty() ? "" : "&locus=" + locus), add_headers);

	QJsonDocument json_doc = QJsonDocument::fromJson(reply.toLatin1());
	QJsonArray file_list = json_doc.array();
	QJsonObject file_object = file_list[0].toObject();


	if (file_object.isEmpty())
	{
		THROW(Exception, "Could not find file info: " + type);
	}

	output = mapJsonObjectToFileLocation(file_object);
	return output;
}


FileLocation FileLocationProviderRemote::mapJsonObjectToFileLocation(QJsonObject obj) const
{	
	return FileLocation {
		obj.value("id").toString(),
		FileLocation::stringToType(obj.value("type").toString()),
		obj.value("filename").toString(),
		obj.value("exists").toBool()
	};
}

FileLocationList FileLocationProviderRemote::mapJsonArrayToFileLocationList(QJsonArray array, bool return_if_missing) const
{
	FileLocationList output {};
	for (int i = 0; i < array.count(); ++i)
	{
		FileLocation loc = mapJsonObjectToFileLocation(array[i].toObject());
		if (loc.exists || return_if_missing)
		{
			output.append(loc);
		}
	}
	return output;
}

FileLocationList FileLocationProviderRemote::getBamFiles(bool return_if_missing) const
{
	return getFileLocationsByType("bam", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getCnvCoverageFiles(bool return_if_missing) const
{
	return getFileLocationsByType("cnvcoverage", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getBafFiles(bool return_if_missing) const
{
	return getFileLocationsByType("baf", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getMantaEvidenceFiles(bool return_if_missing) const
{
	return getFileLocationsByType("mantaevidence", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getCircosPlotFiles(bool return_if_missing) const
{
	return getFileLocationsByType("circosplot", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getVcfFiles(bool return_if_missing) const
{
	return getFileLocationsByType("vcf", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getRepeatExpansionFiles(bool return_if_missing) const
{
	return getFileLocationsByType("repeatexpansion", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getPrsFiles(bool return_if_missing) const
{
	return getFileLocationsByType("prs", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getLowCoverageFiles(bool return_if_missing) const
{
	return getFileLocationsByType("lowcoverage", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getCopyNumberCallFiles(bool return_if_missing) const
{
	return getFileLocationsByType("copynumbercall", return_if_missing);
}

FileLocationList FileLocationProviderRemote::getRohFiles(bool return_if_missing) const
{
	return getFileLocationsByType("roh", return_if_missing);
}

FileLocation FileLocationProviderRemote::getSomaticCnvCoverageFile() const
{
	return getOneFileLocationByType("somaticcnvcoverage", "");
}

FileLocation FileLocationProviderRemote::getSomaticCnvCallFile() const
{
	return getOneFileLocationByType("somaticcnvcall", "");
}

FileLocation FileLocationProviderRemote::getSomaticLowCoverageFile() const
{
	return getOneFileLocationByType("somaticlowcoverage", "");
}

FileLocation FileLocationProviderRemote::getSomaticMsiFile() const
{
	return getOneFileLocationByType("somaticmsi", "");
}

QString FileLocationProviderRemote::getAnalysisPath() const
{
	return "";
}

QString FileLocationProviderRemote::getProjectPath() const
{
	return "";
}
