#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QCommandLineParser>
#include "Log.h"
#include "ServerWrapper.h"
#include "ServerHelper.h"
#include "ServerController.h"

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
						AuthType::NONE,
						"Index page with general information",
						&ServerController::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"",
						QMap<QString, ParamProps>{},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						AuthType::NONE,
						"Size of the index page with general information",
						&ServerController::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"favicon.ico",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::IMAGE_PNG,
						AuthType::NONE,
						"Favicon to avoid warnings from the browser",
						&ServerController::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"info",
						QMap<QString, ParamProps>{},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::NONE,
						"General information about this API",
						&ServerController::serveResourceAsset
					});
	EndpointManager::appendEndpoint(Endpoint{
						"bam",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Name of the BAM file to be served"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_OCTET_STREAM,
						AuthType::NONE,
						"BAM file used for the testing purposes",
						&ServerController::serveResourceAsset
				   });
	EndpointManager::appendEndpoint(Endpoint{
						"bam",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Name of the BAM file to be served"}}
						},
						RequestMethod::HEAD,
						ContentType::APPLICATION_OCTET_STREAM,
						AuthType::NONE,
						"Size of the BAM file used for the testing purposes",
						&ServerController::serveResourceAsset
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"static",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Name of the file to be served"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						AuthType::USER_TOKEN,
						"Static content served from the server root folder (defined in the config file)",
						&ServerController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"static",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Name of the file to be served"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						AuthType::USER_TOKEN,
						"Size of the static content served from the server root folder (defined in the config file)",
						&ServerController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"protected",
						QMap<QString, ParamProps>{
						   {"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Name of the file to be served"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						AuthType::HTTP_BASIC_AUTH,
						"Protected static files",
						&ServerController::serveStaticFromServerRoot
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"temp",
						QMap<QString, ParamProps>{
						   {"id", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Unique id pointing to a file"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						AuthType::USER_TOKEN,
						"Static file served via secure temporary URL",
						&ServerController::serveStaticFromTempUrl
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"temp",
						QMap<QString, ParamProps>{
							{"id", ParamProps{ParamProps::ParamCategory::PATH_PARAM, false, "Unique id pointing to a folder"}},
							{"filename", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true, "Filename in a folder with a temporary URL"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::HEAD,
						ContentType::TEXT_HTML,
						AuthType::USER_TOKEN,
						"Size of the static file served via secure temporary URL",
						&ServerController::serveStaticFromTempUrl
				   });

	EndpointManager::appendEndpoint(Endpoint{
						"help",
						QMap<QString, ParamProps>{
						   {"endpoint", ParamProps{ParamProps::ParamCategory::PATH_PARAM, true,
							"Endpoint path the help is requested for. Help for all endpoints wiil be provided, if this parameter is ommited"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_HTML,
						AuthType::NONE,
						"Help page on the usage of the endpoints",
						&ServerController::serveEndpointHelp
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
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Retrieve file location information for specific file types",
						&ServerController::locateFileByType
					});

	EndpointManager::appendEndpoint(Endpoint{
						"processed_sample_path",
						QMap<QString, ParamProps> {
						   {"ps_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processed sample id"}},
						   {"type", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "File type"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Temporary URL leading to a specific project file (based on the processed sample id)",
						&ServerController::getProcessedSamplePath
					});

	EndpointManager::appendEndpoint(Endpoint{
						"analysis_job_gsvar_file",
						QMap<QString, ParamProps> {
						   {"job_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Analysis job id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"FileLocation object with the information about GSvar for the corresponding analysis job",
						&ServerController::getAnalysisJobGSvarFile
					});

	EndpointManager::appendEndpoint(Endpoint{
						"analysis_job_last_update",
						QMap<QString, ParamProps> {
						   {"job_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Analysis job id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Date and time (in seconds) of the last log file modification for the specific analysis job",
						&ServerController::getAnalysisJobLastUpdate
					});

	EndpointManager::appendEndpoint(Endpoint{
						"analysis_job_log",
						QMap<QString, ParamProps> {
						   {"job_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Analysis job id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Analysis job log file",
						&ServerController::getAnalysisJobLog
					});

	EndpointManager::appendEndpoint(Endpoint{
						"project_file",
						QMap<QString, ParamProps> {
						   {"ps_url_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "An id of a temporary URL pointing to a specific processed sample"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::PUT,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Update an existing project file (GSvar file)",
						&ServerController::saveProjectFile
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_regions",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Processing system regions",
						&ServerController::getProcessingSystemRegions
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_amplicons",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Processing system amplicons",
						&ServerController::getProcessingSystemAmplicons
					});

	EndpointManager::appendEndpoint(Endpoint{
						"ps_genes",
						QMap<QString, ParamProps> {
						   {"sys_id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processing system id"}},
						   {"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Processing system genes",
						&ServerController::getProcessingSystemGenes
					});

	EndpointManager::appendEndpoint(Endpoint{
						"secondary_analyses",
						QMap<QString, ParamProps> {
							{"ps_name", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Processed sample name"}},
							{"type", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, false, "Analysis type"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Secondary analyses list",
						&ServerController::getSecondaryAnalyses
					});

	EndpointManager::appendEndpoint(Endpoint{
						"qbic_report_data",
						QMap<QString, ParamProps> {
							{"filename", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "QBic data report file"}},
							{"id", ParamProps{ParamProps::ParamCategory::GET_URL_PARAM, true, "Location id of the QBic data report file"}},
							{"content", ParamProps{ParamProps::ParamCategory::POST_OCTET_STREAM, false, "QBic report data to be saved in a file"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Save QBic data report files",
						&ServerController::saveQbicFiles
					});
	EndpointManager::appendEndpoint(Endpoint{
						"low_coverage_regions",
						QMap<QString, ParamProps>{
							{"roi", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "Regions of interest"}},
							{"bam_url_id", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "An id of a temporary URL pointing to a BAM file"}},
							{"cutoff", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "Cutoff value"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Calculates low coverage regions",
						&ServerController::calculateLowCoverage
					});
	EndpointManager::appendEndpoint(Endpoint{
						"avg_coverage_gaps",
						QMap<QString, ParamProps>{
							{"low_cov", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "Regions of interest"}},
							{"bam_url_id", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "An id of a temporary URL pointing to a BAM file"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Calculates average coverage for gaps",
						&ServerController::calculateAvgCoverage
					});
	EndpointManager::appendEndpoint(Endpoint{
						"target_region_read_depth",
						QMap<QString, ParamProps>{
							{"regions", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "Regions of interest"}},
							{"bam_url_id", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "An id of a temporary URL pointing to a BAM file"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Calculates target region read depth used in germline report",
						&ServerController::calculateTargetRegionReadDepth
					});

	EndpointManager::appendEndpoint(Endpoint{
						"multi_sample_analysis_info",
						QMap<QString, ParamProps>{
							{"analyses", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "List of available analyses"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Creates a list of analysis names for multi-samples",
						&ServerController::getMultiSampleAnalysisInfo
					});


	EndpointManager::appendEndpoint(Endpoint{
						"upload",
						QMap<QString, ParamProps>{
							{"ps_url_id", ParamProps{ParamProps::ParamCategory::POST_FORM_DATA, true, "An id of a temporary URL pointing to a specific processed sample"}},
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_OCTET_STREAM,
						AuthType::USER_TOKEN,
						"File upload to a folder on the server",
						&ServerController::uploadFile
					});

	EndpointManager::appendEndpoint(Endpoint{
						"login",
						QMap<QString, ParamProps>{
							{"name", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "User name"}},
							{"password", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Password"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::NONE,
						"Secure token generation, the token will be used to access protected resources and to perform  certain API calls",
						&ServerController::performLogin
					});
	EndpointManager::appendEndpoint(Endpoint{
						"session",
						QMap<QString, ParamProps>{
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token to identify the session"}},
						},
						RequestMethod::GET,
						ContentType::APPLICATION_JSON,
						AuthType::USER_TOKEN,
						"Information about the session linked to the given token",
						&ServerController::getSessionInfo
					});
	EndpointManager::appendEndpoint(Endpoint{
						"validate_credentials",
						QMap<QString, ParamProps>{
							{"name", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "User name"}},
							{"password", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Password"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::NONE,
						"Checks if provided GSvar credentials are valid",
						&ServerController::validateCredentials
					});
	EndpointManager::appendEndpoint(Endpoint{
						"db_token",
						QMap<QString, ParamProps>{
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "User name"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Secure token generation for accessing the database credentials",
						&ServerController::getDbToken
					});
	EndpointManager::appendEndpoint(Endpoint{
						"ngsd_credentials",
						QMap<QString, ParamProps>{
							{"dbtoken", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Secure token for the database credentials"}},
							{"secret", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Secret known to a client and to the server"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_JSON,
						AuthType::DB_TOKEN,
						"Sends NGSD credentials to the GSvar client application",
						&ServerController::getNgsdCredentials
					});
	EndpointManager::appendEndpoint(Endpoint{
						"genlab_credentials",
						QMap<QString, ParamProps>{
							{"dbtoken", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Secure token for the database credentials"}},
							{"secret", ParamProps{ParamProps::ParamCategory::POST_URL_ENCODED, false, "Secret known to a client and to the server"}}
						},
						RequestMethod::POST,
						ContentType::APPLICATION_JSON,
						AuthType::DB_TOKEN,
						"Sends Genlab database credentials to the GSvar client application",
						&ServerController::getGenlabCredentials
					});
	EndpointManager::appendEndpoint(Endpoint{
						"logout",
						QMap<QString, ParamProps>{
							{"token", ParamProps{ParamProps::ParamCategory::ANY, false, "Secure token received after a successful login"}}
						},
						RequestMethod::POST,
						ContentType::TEXT_PLAIN,
						AuthType::USER_TOKEN,
						"Secure token invalidation, after this step the token cannot longer be used",
						&ServerController::performLogout
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
		return app.exec();
	}

	Log::info("Restore previous sessions");
	SessionManager::restoreFromFile();
	Log::info("Restore previous URLs");
	UrlManager::restoreFromFile();

	Log::info("SSL version used for the build: " + QSslSocket::sslLibraryBuildVersionString());
	ServerWrapper https_server(https_port_setting);
	if (!https_server.isRunning())
	{
		Log::error("HTTPS is not running. Exiting");
		app.exit(EXIT_FAILURE);
		return app.exec();
	}

	if (!http_port.isEmpty())
	{
		Log::info("HTTP server port has been provided through the command line arguments:" + http_port);
		http_port_setting = https_port.toInt();
	}
	if (http_port_setting == 0)
	{
		Log::error("HTTP port number is invalid");
		app.exit(EXIT_FAILURE);
		return app.exec();
	}
	ServerWrapper http_server(http_port_setting, true);
	if (!http_server.isRunning())
	{
		Log::error("HTTP is not running. Exiting");
		app.exit(EXIT_FAILURE);
		return app.exec();
	}

	ServerHelper::setServerStartDateTime(QDateTime::currentDateTime());
	return app.exec();
}
