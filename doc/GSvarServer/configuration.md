# GSvarServer settings and GSvar settings


## configuring the GSvar server

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
* `queue_update_enabled` - turns on SGE update worker (true/false)
* `megsap_settings_ini` - path to the megSAP settings file (additional settings are extracted from this file)
* `show_raw_request` - flag used for debugging, allows to print out entire HTTP requests in log files(true/false), may significantly increase log sizes, should not be used in productio
* `enable_file_metadata_caching` - turns on/off (true/false) file metadata caching, the cache is needed to reduce the number of calls to a file system (e.g. file size, check if file exists, etc.)
* `file_location_cache_lifespan` - lifespan (seconds) of a cached FileLocation object. When a cached object is accessed, its creation time is reset to the current time.

GSvarServer needs its own database for transient information (sessions, temporary URLs, etc.).  
The database should be a separate instance of MySQL/MariaDB (not NGSD database).
These are the seettings for the database:

* `gsvar_server_db_host` - database host name
* `gsvar_server_db_port` - database port number
* `gsvar_server_db_name` - database name
* `gsvar_server_db_user` - database user name
* `gsvar_server_db_pass` - database user password

## configuring GSvar

To enable the communincation of the GSvar client with the GSvarServer, you have to adapt the `GSvar.ini` file of the client as well.  
See the [GSvar configuration](../GSvar/configuration.md) for details.

--

[back to main page](index.md)
