# Installation of the NGSD

NGSD is an acronym for the *next gen sequencing database*.  
The NGSD manages samples, runs, variants, QC data and genes data for NGS sequencing.

This document describes the installation of the NGSD database.
The installation instructions are based on Ubuntu 16.04 LTS, but should work similarly on other Linux distributions.

These instructions assume that the installation is performed as root. If that is not the case, you have to prepend `sudo ` to all commands that need root privileges. 

## MySQL database setup

The database backend of the NGSD is a MySQL database. To set it up, follow these instructions:

* If not available, install the MySQL package:

		> apt-get install mysql-server

* Log into the server, e.g. with:

		> mysql -u root -p

* Create the NGSD database:

		mysql> create database ngsd;
		mysql> grant all on ngsd.* to 'ngsduser'@'%' identified by '[mysql-password]';
		mysql> exit

## MySQL database optimization

In order to optimize the performance of MySQL for the NGSD, you can adapt/add the following settings in the `/etc/mysql/my.cnf` file:

		[mysqld]
		join_buffer_size = 1M
		query_cache_limit = 16M
		query_cache_size = 256M
		tmp_table_size = 256M
		max_heap_table_size = 256M
		innodb_buffer_pool_size = 2G
		innodb_flush_log_at_trx_commit = 2
		innodb_lock_wait_timeout = 600
		bind_address = 0.0.0.0
		innodb_lru_scan_depth = 512
		max_allowed_packet = 64M
		wait_timeout = 108000


Restart the server:

		> service mysql restart

## Initial import of data

The initial import of database content using ngs-bits.

* If missing, create `ngs-bits/bin/settings.ini` with:

		> cp ngs-bits/bin/settings.ini.example ngs-bits/bin/settings.ini

* Fill in the following NGSD credentials:
	* ngsd_host = `???`
	* ngsd_port = `???`
	* ngsd_name = ngsd
	* ngsd_user = ngsduser
	* ngsd_pass = `[mysql-password]`

* To set up the NGSD tables, use command:

		> ngs-bits/bin/NGSDInit -force [mysql-password]

* Finally, QC terms and gene information from several databases must be imported.  
  Proceed accoding to the instructions of the following tools in the listed order:
	
		> ngs-bits/bin/NGSDImportQC --help
		> ngs-bits/bin/NGSDImportHGNC --help
		> ngs-bits/bin/NGSDImportEnsembl --help
		> ngs-bits/bin/NGSDImportHPO --help
		> ngs-bits/bin/NGSDImportGeneInfo --help
		> ngs-bits/bin/NGSDImportOMIM --help






















