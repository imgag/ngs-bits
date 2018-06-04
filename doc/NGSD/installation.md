
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

Please follow the instructions how to [install the NGSD database](https://github.com/imgag/ngs-bits/blob/master/doc/install_ngsd.md).

## (3) NGSD web frontend setup

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

## (4) GSvar setup (Windows)

GSvar is a variant filtering and reporting tool for Windows that is tightly integrated with the NGSD.
You can download the [pre-built binaries](https://medgen.medizin.uni-tuebingen.de/NGS-downloads/GSvar-current.zip), or you can build the GSvar according to the [Windows installation instructions](../install_win.md).  

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

For more information on GSvar, open the help within GSvar (F1) or use this [link](../GSvar/index.md).


## Next steps

Now you can run your first data analysis based on these [instructions](running_an_analysis.md).





























