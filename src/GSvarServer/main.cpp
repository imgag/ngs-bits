#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QCommandLineParser>
#include "Log.h"
#include "ServerWrapper.h"
#include "ServerHelper.h"
#include "EndpointController.h"
#include "EndpointHandler.h"

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QCoreApplication::setApplicationVersion(SERVER_VERSION);
	Log::setFileName(QCoreApplication::applicationFilePath().replace(".exe", "") + ".log");
	Log::setCMDEnabled(true);
	Log::appInfo();

	QCommandLineParser parser;
	parser.setApplicationDescription("GSvar file server");
	parser.addHelpOption();
	parser.addVersionOption();
	QCommandLineOption httpsServerPortOption(QStringList() << "p" << "port",
			QCoreApplication::translate("main", "HTTPS server port number"),
			QCoreApplication::translate("main", "https_port"));
	parser.addOption(httpsServerPortOption);
	QCommandLineOption httpServerPortOption(QStringList() << "i" << "http_port",
			QCoreApplication::translate("main", "HTTP server port number"),
			QCoreApplication::translate("main", "https_port"));
	parser.addOption(httpServerPortOption);
	QCommandLineOption logLevelOption(QStringList() << "l" << "log",
			QCoreApplication::translate("main", "Log level"),
			QCoreApplication::translate("main", "logging"));
	parser.addOption(logLevelOption);
	parser.process(app);
	QString https_port = parser.value(httpsServerPortOption);
	QString http_port = parser.value(httpServerPortOption);
	QString log_level_option = parser.value(logLevelOption);

	EndpointManager::appendEndpoint(Endpoint{
						"",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						false,
						"Index page with general information",
						&EndpointHandler::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"favicon.ico",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::IMAGE_PNG,
						false,
						"Favicon to avoid warnings from the browser",
						&EndpointHandler::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"info",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						false,
						"General information about this API",
						&EndpointHandler::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"bam",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Name of the BAM file to be served"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_OCTET_STREAM,
						false,
						"BAM file used for the testing purposes",
						&EndpointHandler::serveResourceAsset
				   });
	EndpointManager::appendEndpoint(Endpoint{
						"bam",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Name of the BAM file to be served"}}
						},
						RequestMethod::HEAD,
						ContentType::APPLICATION_OCTET_STREAM,
						false,
						"Size of the BAM file used for the testing purposes",
						&EndpointHandler::serveResourceAsset
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"static",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Name of the file to be served"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						false,
						"Static content served from the server root folder (defined in the config file)",
						&EndpointController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"static",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Name of the file to be served"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						false,
						"Size of the static content served from the server root folder (defined in the config file)",
						&EndpointController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"protected",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Name of the file to be served"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						true,
						"Protected static files",
						&EndpointController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"cache",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Name of the file to be served"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						false,
						"Static content served from the server cache",
						&EndpointController::serveStaticFileFromCache
				   });
	EndpointManager::appendEndpoint(Endpoint{
						"temp",
						QMap<QString, ParamProps>{
						   {"id", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Unique id pointing to a file"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						false,
						"Static file served via secure temporary URL",
						&EndpointController::serveStaticForTempUrl
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"temp",
						QMap<QString, ParamProps>{
							{"id", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Unique id pointing to a folder"}},
							{"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Filename in a folder with a temporary URL"}},
							{"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						false,
						"Size of the static file served via secure temporary URL",
						&EndpointController::serveStaticForTempUrl
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"help",
						QMap<QString, ParamProps>{
						   {"endpoint", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true,
							"Endpoint path the help is requested for. Help for all endpoints wiil be provided, if this parameter is ommited"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						false,
						"Help page on the usage of the endpoints",
						&EndpointController::serveEndpointHelp
					});

	EndpointManager::appendEndpoint(Endpoint{
						"file_location",
						QMap<QString, ParamProps> {
						   {"ps_url_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "An id of a temporary URL pointing to a specific processed sample"}},
						   {"type", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Format of the requested file(s)"}},
						   {"path", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Returns an absolute path on the server, if set to 'absolute'"}},
						   {"locus", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Locus for repeat expansion image"}},
						   {"multiple_files", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Flag indicating that we expect a single file/list of files"}},
						   {"return_if_missing", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Return file info, if the file is missing"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						false,
						"Retrieve file location information for specific file types",
						&EndpointHandler::locateFileByType
					});

	EndpointManager::appendEndpoint(Endpoint{
						"processed_sample_path",
						QMap<QString, ParamProps> {
						   {"ps_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processed sample id"}},
						   {"type", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "File type"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						false,
						"Temporary URL leading to a specific project file (based on the processed sample id)",
						&EndpointHandler::getProcessedSamplePath
					});

	EndpointManager::appendEndpoint(Endpoint{
						"analysis_job_gsvar_file",
						QMap<QString, ParamProps> {
						   {"job_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Analysis job id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						false,
						"FileLocation object with the information about GSvar for the corresponding analysis job",
						&EndpointHandler::getAnalysisJobGSvarFile
					});

	EndpointManager::appendEndpoint(Endpoint{
						"project_file",
						QMap<QString, ParamProps> {
						   {"ps_url_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "An id of a temporary URL pointing to a specific processed sample"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::PUT,
						ContentType::APPLICATION_JSON,
						false,
						"Update an existing project file (GSvar file)",
						&EndpointHandler::saveProjectFile
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_regions",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						false,
						"Processing system regions",
						&EndpointHandler::getProcessingSystemRegions
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_amplicons",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						false,
						"Processing system amplicons",
						&EndpointHandler::getProcessingSystemAmplicons
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_genes",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						false,
						"Processing system genes",
						&EndpointHandler::getProcessingSystemGenes
					});

	EndpointManager::appendEndpoint(Endpoint{
						"secondary_analyses",
						QMap<QString, ParamProps> {
							{"ps_name", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processed sample name"}},
							{"type", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Analysis type"}},
							{"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						false,
						"Secondary analyses list",
						&EndpointHandler::getSecondaryAnalyses
					});

	EndpointManager::appendEndpoint(Endpoint{
						"qbic_report_data",
						QMap<QString, ParamProps> {
							{"filename", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "QBic data report file"}},
							{"id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Location id of the QBic data report file"}},
							{"content", ParamProps{ParamProps::ParamCategory::POST_OCTET_STREAM, false, "QBic report data to be saved in a file"}},
							{"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_JSON,
						false,
						"Save QBic data report files",
						&EndpointHandler::saveQbicFiles
					});

	EndpointManager::appendEndpoint(Endpoint{
						"upload",
						QMap<QString, ParamProps>{
							{"ps_url_id", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "An id of a temporary URL pointing to a specific processed sample"}},
							{"token", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_OCTET_STREAM,
						false,
						"File upload to a folder on the server",
						&EndpointHandler::uploadFile
					});

	EndpointManager::appendEndpoint(Endpoint{
							"login",
						QMap<QString, ParamProps>{
							{"name", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "User name"}},
							{"password", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Password"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						false,
						"Secure token generation, the token will be used to access protected resources and to perform  certain API calls",
						&EndpointHandler::performLogin
					});
	EndpointManager::appendEndpoint(Endpoint{
						"logout",
						QMap<QString, ParamProps>{
							{"token", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						false,
						"Secure token invalidation, after this step the token cannot longer be used",
						&EndpointHandler::performLogout
					});



	int https_port_setting = ServerHelper::getNumSettingsValue("https_server_port");
	int http_port_setting = ServerHelper::getNumSettingsValue("http_server_port");

	if (!https_port.isEmpty())
	{
		Log::info("HTTPS server port has been provided through the command line arguments:" + https_port);
		https_port_setting = https_port.toInt();
	}
	if (https_port_setting == 0)
	{
		Log::error("HTTPS port number is invalid");
		app.exit(EXIT_FAILURE);
	}

	Log::info("SSL version used for build: " + QSslSocket::sslLibraryBuildVersionString());
	ServerWrapper https_server(https_port_setting);

	if (!http_port.isEmpty())
	{
		Log::info("HTTP server port has been provided through the command line arguments:" + http_port);
		http_port_setting = https_port.toInt();
	}

	if (http_port_setting == 0)
	{
		Log::error("HTTP port number is invalid");
		app.exit(EXIT_FAILURE);
	}
	ServerWrapper http_server(http_port_setting, true);

	return app.exec();
}
