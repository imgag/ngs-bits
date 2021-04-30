# Installation of the NGSD

NGSD is an acronym for the *next gen sequencing database*.  
The NGSD manages samples, runs, variants, QC data and genes data for NGS sequencing.

This document describes the installation of the NGSD database.
The installation instructions are based on Ubuntu 18.04 LTS, but should work similarly on other Linux distributions.

These instructions assume that the installation is performed as root. If that is not the case, you have to prepend `sudo ` to all commands that need root privileges. 

## MySQL database setup

The database backend of the NGSD is a MySQL database. To set it up, follow these instructions:

* If not available, install the MySQL package:

		> sudo apt-get install mysql-server

* Log into the server, e.g. with:

		> sudo mysql -u root

* Create the NGSD database:

		mysql> create database ngsd;
		mysql> grant all on ngsd.* to 'ngsduser'@'%' identified by '[mysql-password]';
		mysql> exit

## MySQL database optimization

In order to optimize the performance of MySQL for the NGSD, you can adapt/add the following settings in the `/etc/mysql/my.cnf` file:

		[mysqld]
		innodb_buffer_pool_instances = 2
		innodb_buffer_pool_size = 32G
		innodb_flush_log_at_trx_commit = 2
		innodb_lock_wait_timeout = 1000
		innodb_lru_scan_depth = 512
		join_buffer_size = 16M
		max_allowed_packet = 64M
		max_connections = 300
		max_heap_table_size = 256M
		query_cache_limit = 64M
		query_cache_size = 256M
		tmp_table_size = 256M
		wait_timeout = 108000


Restart the server:

		> sudo service mysql restart

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
		> ngs-bits/bin/NGSDImportORPHA --help
		
## Export NGSD annotation data

The NGSD variant counts for germline and somatic as well as the gene info can be exported to a VCF/BED file using NGSDExportAnnotationData. 

* For germline:

		> ngs-bits/bin/NGSDExportAnnotationData -variants [vcf_output_file] -genes [bed_file]
* For somatic:

		> ngs-bits/bin/NGSDExportAnnotationData -variants [somatic_vcf_output_file] -mode somatic
		
The resulting VCF files have to be sorted (e.g. using `VcfStreamSort`) and then gzipped and indexed to be used as annotation source:

		> ngs-bits/bin/VcfStreamSort -in [unsorted_vcf] -out [sorted_vcf]
		> bgzip -c [sorted_vcf] > [sorted_vcf_gzip]
		> tabix -p vcf [sorted_vcf_gzip]
		
For variant annotation `VcfAnnotateFromVcf` and `VcfAnnotateFromBed` can be used. 
