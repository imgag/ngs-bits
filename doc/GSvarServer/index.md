## GSvarServer

The server supports only HTTPS protocol. A certificate-key pair has to be generated:

sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout dev-selfsigned.key -out dev-selfsigned.crt\n

## Qt debug statements
- Open qtlogging.ini in /etc/xdg/QtProject/ (create a new empty file, if it does not exist)
- Add (or modify accordingly) the following config
[Rules]
*.debug=true
qt.*.debug=false
