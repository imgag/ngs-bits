# Installation of the NGSD

NGSD is an acronym for the *next generation sequencing database*.  
The NGSD manages samples, runs, variants, QC data as well as basic data data like genes and transcripts .

This document describes the installation of the NGSD database.
The installation instructions are based on Ubuntu 20.04 LTS, but should work similarly on other Linux distributions.

These instructions assume that the installation is performed as root. If that is not the case, you have to prepend `sudo ` to all commands that need root privileges. 

## SQL database setup

The database backend of the NGSD is a MariaDB database. To set it up, follow these instructions:

* If not available, install the SQL server:

		> sudo apt-get install mariadb-server

* Log into the server, e.g. with:

		> sudo mysql -u root

* Create the NGSD production and test instance:

		mysql> create database ngsd;
		mysql> grant all on ngsd.* to 'ngsduser'@'%' identified by '[password]';
		mysql> create database ngsd_test;
		mysql> grant all on ngsd_test.* to 'ngsdtestuser'@'%' identified by '[password]';
		mysql> exit

## database optimization

In order to optimize the performance of the SQL server for the NGSD, you can adapt/add the following settings in the `/etc/mysql/my.cnf` file:

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

## Initial import of gene, transcript and disease data

The initial import of database content using ngs-bits.

* If missing, create `ngs-bits/bin/settings.ini` with:

		> cp ngs-bits/bin/settings.ini.example ngs-bits/bin/settings.ini

* Fill in the following NGSD credentials:
	* ngsd_host = `[host]`
	* ngsd_port = 3306
	* ngsd_name = ngsd
	* ngsd_user = ngsduser
	* ngsd_pass = `[password]`

* To set up the NGSD tables, use command:

		> ngs-bits/bin/NGSDInit

* Finally, QC terms and gene information from several databases must be imported.  
  Proceed accoding to the instructions of the following tools in the listed order:
	
		> ngs-bits/bin/NGSDImportQC --help
		> ngs-bits/bin/NGSDImportHGNC --help
		> ngs-bits/bin/NGSDImportEnsembl --help
		> ngs-bits/bin/NGSDImportHPO --help
		> ngs-bits/bin/NGSDImportOncotree --help
		> ngs-bits/bin/NGSDImportGeneInfo --help
		> ngs-bits/bin/NGSDImportOMIM --help
		> ngs-bits/bin/NGSDImportORPHA --help


## Update of gene, transcript and disease data

To keep gene, transcript and disease data in NGSD up-to-date, you need to perform regular updates.  
To do that, update ngs-bits and re-run these import tools:

		> ngs-bits/bin/NGSDImportQC --help
		> ngs-bits/bin/NGSDImportHGNC --help
		> ngs-bits/bin/NGSDImportEnsembl --help
		> ngs-bits/bin/NGSDImportHPO --help
		> ngs-bits/bin/NGSDImportOncotree --help
		> ngs-bits/bin/NGSDImportGeneInfo --help
		> ngs-bits/bin/NGSDImportOMIM --help
		> ngs-bits/bin/NGSDImportORPHA --help

After the update of gene and HPO data, you should run the maintenance tasks `Replace obsolete gene symbols` and `Replace obsolete HPO terms` in GSvar (NGSD > Admin > Maintenance).

*Note:  
Make sure to run the tools with the `-force` parameter to override the existing data!*

*Note:  
You should perform a import into a test database first, to make sure the import works.   
After the test import, you can check the NGSD data using GSvar (NGSD > Admin > Maintenance > Compare base data of test and production).*

*Note:  
The data links in these tools are updated roughly every 3 months.  
We recommend to leave the data links unchanged, even if there is newer data available.  
For the default links, we have verify that the linked data is in valid format for the import.  
If you change the data links to perform an update to with newer data, there is a risk that the data format has changed and the imported data is corrupted or incomplete.*

## Export NGSD annotation data

The NGSD variant counts for germline and somatic as well as the gene info can be exported to a VCF/BED file using NGSDExportAnnotationData. 

* For germline small variants:

		> ngs-bits/bin/NGSDExportAnnotationData -variants [vcf_output_file] -genes [bed_file]

* For germline structural variants:

		> ngs-bits/bin/NGSDExportSV -out_folder [export_folder]

* For somatic small variants:

		> ngs-bits/bin/NGSDExportAnnotationData -variants [somatic_vcf_output_file] -mode somatic
		
		
The resulting VCF files have to be sorted (e.g. using `VcfStreamSort`) and then gzipped and indexed to be used as annotation source:

		> ngs-bits/bin/VcfStreamSort -in [unsorted_vcf] -out [sorted_vcf]
		> bgzip -c [sorted_vcf] > [sorted_vcf_gzip]
		> tabix -p vcf [sorted_vcf_gzip]
		
For variant annotation `VcfAnnotateFromVcf` and `VcfAnnotateFromBed` can be used. 
