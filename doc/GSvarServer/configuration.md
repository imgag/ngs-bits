# GSvarServer settings and GSvar settings

## Configuring the GSvar server

To run GSvarServer, you have to configure it using the `GSvarServer.ini` file.  
These are the most important config parameters:

* `ssl_certificate` - location of your SSL certificate
* `ssl_key` - location of your private key
* `server_port` - port used by the server
* `server_host` - domain name used be the server
* `url_lifetime` - lifespan (seconds) of a temporary URL genereated by the server
* `session_duration` - valid period (seconds) of a user session
* `threads` - number of threads used for parallel calculations
* `thread_timeout` - request worker thread timeout in seconds
* `thread_count` - thread pool size of request workers
* `socket_read_timeout` - socket read timeout (seconds)
* `socket_write_timeout` - socket write timeout (seconds)
* `socket_encryption_timeout` - socket encryption wait timeout (seconds)
* `server_root` - root folder used to server static content (used for development only)
* `allow_folder_listing` - enables viewing the list of folder items (used for development only)
* `ngsd_host` - NGSD host name
* `ngsd_port` - NGSD port number
* `ngsd_name` - NGSD database name
* `ngsd_user` - NGSD database user name
* `ngsd_pass` - NGSD user password
* `queue_update_enabled` - turns on SGE/Slurm update worker (true/false). SGE or Slurm must be setup for this option (see [Slurm installation](install_slurm.md)).
* `qe_api_base_url` - URL for the queuing engine HTTP API: submitting jobs, updating running jobs, cheking completed jobs, deleting jobs without interacting directly with the queuing engine
* `qe_secure_token` - a token that is needed to access the queuing engine API server
* `megsap_settings_ini` - path to the megSAP settings file (additional settings are extracted from this file)
* `show_raw_request` - flag used for debugging, allows to print out entire HTTP requests in log files(true/false), may significantly increase log sizes, should not be used in productio
* `enable_file_metadata_caching` - turns on/off (true/false) file metadata caching, the cache is needed to reduce the number of calls to a file system (e.g. file size, check if file exists, etc.)
* `file_location_cache_lifespan` - lifespan (seconds) of a cached FileLocation object. When a cached object is accessed, its creation time is reset to the current time.

GSvarServer needs its own database for transient information (sessions, temporary URLs, etc.).  
You have to create a separate database used only by GSvarServer (don't use NGSD).
These are the seettings for the database:

* `gsvar_server_db_host` - database host name
* `gsvar_server_db_port` - database port number
* `gsvar_server_db_name` - database name
* `gsvar_server_db_user` - database user name
* `gsvar_server_db_pass` - database user password

## Using a custom queuing engine server

Please refer to the [queuing engine server documenation](qe_api.md) to learn more.

## Configuring GSvar

To enable the communincation of the GSvar client with the GSvarServer, you have to adapt the `GSvar.ini` file of the client as well.  
See the [GSvar configuration](../GSvar/configuration.md) for details.

--

[back to main page](index.md)
