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
	return FileLocation(); //Alexandr
}

FileLocation FileLocationProviderRemote::getAnalysisSvFile() const
{
	return FileLocation(); //Alexandr
}

FileLocation FileLocationProviderRemote::getAnalysisCnvFile() const
{
	return FileLocation(); //Alexandr
}

FileLocation FileLocationProviderRemote::getAnalysisMosaicCnvFile() const
{
	return FileLocation(); //Alexandr
}

FileLocation FileLocationProviderRemote::getAnalysisUpdFile() const
{
	return FileLocation(); // Alexandr
}

FileLocation FileLocationProviderRemote::getRepeatExpansionImage(QString /*locus*/) const
{
	return FileLocation(); // Alexandr
}

FileLocationList FileLocationProviderRemote::requestFileLocationsByType(PathType type, bool return_if_missing) const
{
	FileLocationList output {};
	if (sample_id_.isEmpty())
	{
		THROW(ArgumentException, "File name has not been specified")
		return output;
	}

	HttpHeaders add_headers;
	add_headers.insert("Accept", "application/json");
	QString reply = HttpRequestHandler(HttpRequestHandler::NONE).get("https://" + server_host_ + ":" + QString::number(server_port_) + "/v1/file_location?ps=" + sample_id_ + "&type=" + FileLocation::typeToString(type), add_headers);

	QJsonDocument json_doc = QJsonDocument::fromJson(reply.toLatin1());
	QJsonArray file_list = json_doc.array();

	if (file_list.count() == 0)
	{
		THROW(Exception, "Could not find file info: " + FileLocation::typeToString(type));
	}

	qDebug() << "Requested files:" << file_list;
	output = mapJsonArrayToFileLocationList(file_list, return_if_missing);
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

FileLocationList FileLocationProviderRemote::getBamFiles(bool return_if_missing) const //Alexandr
{
	return requestFileLocationsByType(PathType::BAM, return_if_missing);
}

FileLocationList FileLocationProviderRemote::getCnvCoverageFiles(bool return_if_missing) const //Alexandr
{
	return requestFileLocationsByType(PathType::COPY_NUMBER_RAW_DATA, return_if_missing);
}

FileLocationList FileLocationProviderRemote::getBafFiles(bool return_if_missing) const //Alexandr
{
	return requestFileLocationsByType(PathType::BAF, return_if_missing);
}

FileLocationList FileLocationProviderRemote::getMantaEvidenceFiles(bool return_if_missing) const //Alexandr
{
	return requestFileLocationsByType(PathType::MANTA_EVIDENCE, return_if_missing);
}

FileLocationList FileLocationProviderRemote::getCircosPlotFiles(bool return_if_missing) const //Alexandr
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderRemote::getVcfFiles(bool /*return_if_missing*/) const //Alexandr
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderRemote::getRepeatExpansionFiles(bool /*return_if_missing*/) const //Alexandr
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderRemote::getPrsFiles(bool /*return_if_missing*/) const //Alexandr
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderRemote::getLowCoverageFiles(bool /*return_if_missing*/) const //Alexandr
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderRemote::getCopyNumberCallFiles(bool /*return_if_missing*/) const //Alexandr
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderRemote::getRohFiles(bool /*return_if_missing*/) const //Alexandr
{
	return FileLocationList{};
}

FileLocation FileLocationProviderRemote::getSomaticCnvCoverageFile() const //Alexandr
{
	return FileLocation();
}

FileLocation FileLocationProviderRemote::getSomaticCnvCallFile() const //Alexandr
{
	return FileLocation();
}

FileLocation FileLocationProviderRemote::getSomaticLowCoverageFile() const //Alexandr
{
	return FileLocation();
}

FileLocation FileLocationProviderRemote::getSomaticMsiFile() const //Alexandr
{
	return FileLocation();
}

QString FileLocationProviderRemote::getAnalysisPath() const
{
	return "";
}

QString FileLocationProviderRemote::getProjectPath() const
{
	return "";
}
