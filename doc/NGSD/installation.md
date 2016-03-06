
#Installation of the NGSD

NGSD is an acronym for the *next gen sequencing database*.  
The NGSD manages samples, runs, variants and QC data for NGS sequencing.

This document describes the installation of the NGSD database and the GSvar tool.  
The installation instructions are based on Ubuntu 14.04 LTS, but should work similarly on other Linux distributions.

## (0) Installation of dependencies

First we need to make sure that we have all packages required the rest of the installation:

	> apt-get install subversion git php5 php5-mysql g++ maven libncurses5-dev qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql cmake python python-matplotlib tabix

##(1) Setup of the analysis pipeline

First, we can check out the analysis pipeline code (contact us for the password):

	> svn co http://saas1305xh.saas-secure.com/svn/php --username nightly --password [password]

Next, we need to download and build some open-source tools that our pipeline relies on:

	> cd php/data/
	> chmod 755 download_*.sh
	> ./download_tools.sh

Then, we need to download and index the reference genome:
	
	> ./download_hg19.sh

Next, we need to download and convert some open-source databases that our pipeline relies on:
	
	> ./download_dbs.sh

Finally, we need to configure the pipeline:

	> cp setting.ini.default setting.ini

Optionally, we can test the installation (takes an hour):

	> make test_pipeline_a test_pipeline_x


##(2) Setup of the MySQL database

The database backend of the NGSD is a MySQL database. To set it up, follow these instructions:

* Install the MySQL package

		> apt-get install mysql-server

* Log into the server using

		> mysql -u root -p

* Create the NGSD database:

		mysql> create database ngsd;
		mysql> grant all on ngsd.* to 'ngsduser' identified by '[password]';

* In oder to optimize the performance of MySQL for the NGSD, adapt/add the following settings in the `/etc/mysql/my.cnf` file:

		[mysqld]
		join_buffer_size = 1M
		query_cache_limit = 16M
		query_cache_size = 256M
		tmp_table_size = 256M
		max_heap_table_size = 256M
		innodb_buffer_pool_size = 2G
		innodb_flush_log_at_trx_commit = 2

* Restart the server:

		> service mysql restart

##(3) Setup of MySQL tables

The initial setup of the database tables is done using one of the ngs-bits tools:

* First you need to configure ngs-bits, which we installed to `php/data/tools/ngs-bits/`. Copy the `bin/settings.ini.example` to `bin/settings.ini` and fill in at least the following items:
<table>
	<tr>
		<td>reference\_genome</td>
		<td>Path to the indexed reference genome FASTA file, see (1).</td>
	</tr>
	<tr>
		<td>ngsd\_host<br>ngsd\_port<br>ngsd\_name<br>ngsd\_user<br>ngsd\_pass</td>
		<td>MySQL database credentials, see (2).</td>
	</tr>
</table>
* To initialize the NGSD tables, use command:

		> bin/NGSDInit -force [MySQL password]

##(4) Setup of the NGSD web frontend

The NGSD web frontend is that main GUI for the NGSD. It runs on a apache server.
Install like that:

* First, install the apache server:

		> apt-get install apache2 libapache2-mod-php5

* Check out the NGSD source code like that (contact us for the password):
	
		> cd /var/www/html/
		> svn co http://saas1305xh.saas-secure.com/svn/DB/http --username nightly --password [password]
		> mv http DB

* Now we need to configure the NGSD:

  Copy the `DB/settings.ini.example` to `DB/settings.ini` and adapt the settings:
		
		[general]
		basedir = "/DB/"

  Then, copy the `DB/sites/NGSD/settings.ini.example` to `DB/sites/NGSD/settings.ini` and adapt the settings:
		
	<table>
		<tr>
			<td>[paths]<br>ngs-bits</td>
			<td>The path to the ngs-bits `bin` folder.</td>
		</tr>
		<tr>
			<td>[database]<br>db_host<br>db_name<br>db_user<br>db_pass</td>
			<td>MySQL database credentials, see (2).</td>
		</tr>
	</table>
* **TODO: Christopher fragen?**
* Now, the NGSD database can be accessed at `http://localhost/DB/NGSD/`.

##(5) Setup of GSvar (Windows)

GSvar is a variant filtering and reporting tool for Windows that is tightly integrated with the NGSD.

* To build the binaries, see the [Windows installation instructions](../install_win.md).  
  Alternatively, contact us for pre-built portable binaries.
* After building GSvar, you need to configure it. Copy the `bin\settings.ini.example` to `bin\settings.ini` and fill in at least the following items:
<table>
	<tr>
		<td>reference\_genome</td>
		<td>Path to the indexed reference genome FASTA file, see (1).</td>
	</tr>
	<tr>
		<td>ngsd\_host<br>ngsd\_port<br>ngsd\_name<br>ngsd\_user<br>ngsd\_pass</td>
		<td>MySQL database credentials, see (2).</td>
	</tr>
	<tr>
		<td>NGSD</td>
		<td>The URL of the NGSD webservice</td>
	</tr>
</table>

For more information on GSvar, open the help within GSvar (F1) or use this [link](../GSvar/index.md).


