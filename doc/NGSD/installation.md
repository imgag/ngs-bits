
#Installation of the NGSD

NGSD is an acronym for the *next gen sequencing database*.  
The NGSD manages samples, runs, variants and QC data for NGS sequencing.

This document describes the installation of the NGSD database and the GSvar tool.  
The installation instructions are based on Ubuntu 14.04 LTS, but should work similarly on other Linux distributions.

These instructions assume that the installation is performed as root. If that is not the case, you have to prepend `sudo ` to all commands that need root privileges. 

## (0) Installation of dependencies

First we need to make sure that we have all packages required the rest of the installation.

For Ubuntu 14.04 install these packages:

	> apt-get install subversion git php5 php5-mysql php5-gd g++ maven libncurses5-dev qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql cmake python python-matplotlib tabix

For Ubuntu 16.04 install these packages:

	> apt-get install subversion git php7.0-cli php7.0-mysql php7.0-xml php7.0-gd g++ maven libncurses5-dev qt5-default libqt5xmlpatterns5-dev libqt5sql5-mysql cmake python python-matplotlib tabix

##(1) Setup of the analysis pipeline

First, we can check out the analysis pipeline code:

	> git clone --recursive https://github.com/imgag/megSAP.git

Next, we need to download and build some open-source tools that our pipeline relies on:

	> cd php/data/
	> chmod 755 download_*.sh
	> ./download_tools.sh

Now, we need to configure the pipeline. First, copy the default settings file:

	> cp php/settings.ini.default php/settings.ini

Then, choose a password for the MySQL database which we will create in the next step and set the password in the appropriate line: 

	[mysql-databases]
	db_pass['NGSD'] = "[mysql-password]"

Next, we need to download and index the reference genome:
	
	> ./download_hg19.sh

Now, we need to configure the the reference genome used by the ngs-bits tools. See (3) for details

Finally, we need to download and convert some open-source databases that our pipeline relies on:

	> ./download_dbs.sh

**Note:** OMIM, HGMD and COSMIC are not downloaded automatically because of license issues. If you have the license for those databases, download/convert them according to the commented sections in the download script.

##(2) Setup of the MySQL database for NGSD

The database backend of the NGSD is a MySQL database. To set it up, follow these instructions:

* Install the MySQL package

		> apt-get install mysql-server

* Log into the server using

		> mysql -u root -p

* Create the NGSD database:

		mysql> create database ngsd;
		mysql> grant all on ngsd.* to 'ngsduser' identified by '[mysql-password]';
		mysql> exit

* In order to optimize the performance of MySQL for the NGSD, adapt/add the following settings in the `/etc/mysql/my.cnf` file:

		[mysqld]
		join_buffer_size = 1M
		query_cache_limit = 16M
		query_cache_size = 256M
		tmp_table_size = 256M
		max_heap_table_size = 256M
		innodb_buffer_pool_size = 2G
		innodb_flush_log_at_trx_commit = 2
		bind_address = 0.0.0.0

* Restart the server:

		> service mysql restart

##(3) Setup of MySQL tables for NGSD

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

		> bin/NGSDInit -force [mysql-password]

* Then, the QC ontology needs to be imported:
	
		> php php/src/NGS/db_update_qcml.php

* Finally, gene information from several databases must be imported.  
  Proceed accoding to the instructions of the following tools in the listed order:
	
		> bin/NGSDImportHGNC --help
		> bin/NGSDImportUCSC --help
		> bin/NGSDImportHPO --help

##(4) Setup of the NGSD web frontend

The NGSD web frontend is that main GUI for the NGSD. It runs on a apache server.
Install like that:

* First, install the apache server:

		> apt-get install apache2 libapache2-mod-php5
		> a2enmod rewrite

* Check out the NGSD source code like that (contact us for credentials):
	
		> cd /var/www/html/
		> svn co http://saas1305xh.saas-secure.com/svn/DB/http --username [user] --password [password]
		> mv http DB

* Now we need to configure the NGSD:

  Copy the `DB/.htaccess.example` to `DB/.htaccess`.

  Copy the `DB/settings.ini.example` to `DB/settings.ini`.

  Copy the `DB/sites/NGSD/settings.ini.example` to `DB/sites/NGSD/settings.ini` and adapt the settings:
		
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

	Apapt `/etc/apache2/apache2.conf` like that:
        
		<Directory /var/www/>
        	Options Indexes FollowSymLinks MultiViews
            AllowOverride All
            Require all granted
 
	        php_flag short_open_tag On
	        php_flag display_errors On
	        php_flag html_errors  On
	        php_value memory_limit 2000M
	        php_value session.cache_expire 1440
	        php_value session.gc_maxlifetime 86400
        </Directory>

* Restart the server:

		> service apache2 restart

* Now, the NGSD database can be accessed at `http://localhost/DB/NGSD/`.

##(5) Setup of GSvar (Windows)

GSvar is a variant filtering and reporting tool for Windows that is tightly integrated with the NGSD.
You can download the [pre-built binaries](https://medgen.medizin.uni-tuebingen.de/NGS-downloads/GSvar-current.zip), or you can build the GSvar according to the [Windows installation instructions](../install_win.md).  

After building GSvar, you need to configure it:

 * Copy the `bin\settings.ini.example` to `bin\settings.ini` and fill in at least the following items:
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
			<td>The URL of the NGSD web frontend</td>
		</tr>
	</table>
 * Copy the `bin\GSvar.ini.example` to `bin\GSvar.ini`.

For more information on GSvar, open the help within GSvar (F1) or use this [link](../GSvar/index.md).


##Next steps

Now you can run your first data analysis based on these [instructions](running_an_analysis.md).





















