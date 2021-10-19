## GSvarServer

The server supports both HTTP and HTTPS protocols. HTTP is considered to be insecure for the production environment. We recommend to use it for debugging during the development
process. To enable a proper HTTPS support, a certificate-key pair has to be generated:

sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout dev-selfsigned.key -out dev-selfsigned.crt\n

## Local development environment

GSvarServer requires a MySQL database. You may have your own local instance of MySQL with test data:

docker run --name my-own-mysql -e MYSQL_ROOT_PASSWORD=mypass123 -d mysql:5.7

To start PhpMyAdmin, run this command:

docker run --name my-own-phpmyadmin -d --link my-own-mysql:db -p 8081:80 phpmyadmin/phpmyadmin

Having a local Apache server instance may be helpful in the debugging process. To run it in a Docker container, execute the following command (current directory will be
used as a server root):

docker run -dit --name my-apache-app -p 8080:80 -v "$PWD":/usr/local/apache2/htdocs/ httpd:2.4

## Qt debug statements
- Open qtlogging.ini in /etc/xdg/QtProject/ (create a new empty file, if it does not exist)
- Add (or modify accordingly) the following config
[Rules]
*.debug=true
qt.*.debug=false
