
# Installation of megSAP, NGSD and GSvar

megSAP a collection of data analysis pipelines for NGS data.  
NGSD (*next gen sequencing database*) is our database for NGS data, which is required for a complete analysis with megSAP.  
GSvar is a variant viewer that is used to visualize the analysis results of the megSAP pipelines.

These instructions assume that the installation is performed as root. If that is not the case, you have to prepend `sudo ` to all commands that need root privileges. 

## (1) megSAP setup

First, we need to [install the megSAP pipeline](https://github.com/imgag/megSAP).

Now, we need to configure the pipeline. First, copy the default settings file:

	> cp megSAP/settings.ini.default megSAP/settings.ini

Then, choose a password for the MySQL database which we will create in the next step and set the password in the appropriate line: 

	[mysql-databases]
	db_pass['NGSD'] = "[mysql-password]"

## (2) NGSD setup

Please follow the instructions how to [install the NGSD database](install_ngsd.md).

## (3) NGSD web frontend setup

The NGSD web frontend is that main GUI for the NGSD. It runs on a apache server.
Install like that:

* First, install the apache server:

		> apt-get install apache2 libapache2-mod-php5
		> a2enmod rewrite

* Check out the NGSD source code like that (contact us for credentials):
	
		> cd /var/www/html/
		> git clone https://www.medizin.uni-tuebingen.de/gitmgen/r/DB.git


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

## (4) GSvar setup (Windows)

GSvar is a variant filtering and reporting tool for Windows that is tightly integrated with the NGSD.
You can download the [pre-built binaries](https://medgen.medizin.uni-tuebingen.de/NGS-downloads/GSvar-current.zip), or you can build the GSvar according to the [Windows installation instructions](install_win.md).  

After building GSvar, you need to configure it:

 * Copy the `bin\settings.ini.example` to `bin\settings.ini` and fill in at least the following items:
	<table>
		<tr>
			<td>reference_genome</td>
			<td>Path to the indexed reference genome FASTA file, see (1).</td>
		</tr>
		<tr>
			<td>ngsd_host<br>ngsd_port<br>ngsd_name<br>ngsd_user<br>ngsd_pass</td>
			<td>MySQL database credentials, see (2).</td>
		</tr>
		<tr>
			<td>NGSD</td>
			<td>The URL of the NGSD web frontend</td>
		</tr>
	</table>
 * Copy the `bin\GSvar.ini.example` to `bin\GSvar.ini`.

For more information on GSvar, open the help within GSvar (F1) or use this [link](GSvar/index.md).


## (5) Running a NGS data analysis

### Downloading test data

For the first data anlysis you first some test data.  
You can download an example dataset of the NIST reference sample NA12878 [here](https://medgen.medizin.uni-tuebingen.de/NGS-downloads/NA12878_01.zip).

### Running an analysis

The analysis pipeline assumes that that all data to analyze resides in a sample folder as produced by Illumina's [bcl2fastq](http://support.illumina.com/sequencing/sequencing_software/bcl2fastq-conversion-software.html) tool. If that is the case, the whole analysis is performed with one command.  
For example, the command to analyze the NA12878 test data is this:

	php php/src/Pipelines/analyze.php -folder Sample_NA12878_01 -name NA12878_01 -system hpHBOCv5.ini -steps ma,vc,an

After the data analysis, the sample folder contains BAM and VCF (gzipped) files as expected.  
Additionally a GSvar file is created (open with the GSvar tool) and several qcML files that contain QC data (open with a browser).

### Importing variant/QC data into the NGSD

In order to import variants and QC values of a sample into the NGSD, you have to create a processed sample in the NGSD first:

 * First, go to the `Admin section` and create:
  * A `device` (sequencer).
  * A `sender` for the sample.
 * Create a `project`:
  * Use 'admin' as internal coordinator.
 * Create a `sequencing run`:
  * The 'recipe' is the read length(s), e.g. '100+8+100'
  * Use the 'device' you created above.
 * Create a `sample`:
  * Name it 'NA12878'.
  * Use the 'sender' you created above.
  * Select 'DNA' as sample type.
  * Select 'human' as species.
 * Create a `processing system`:
  * For this stop, use the information from the `hpHBOC.ini` file download above.
 * Create a `processed sample`:
  * Use 'NA12878' as sample.
  * Use the 'project' you created above.
  * Use the 'run' you created above.
  * Use the 'processing system' you created above.

Now, you can import the variant and QC data of the sample `NA12878_01`into the NGSD using the following command:

	php php/src/Pipelines/analyze.php -folder Sample_NA12878_01 -name NA12878_01 -system hpHBOCv5.ini -steps db
































