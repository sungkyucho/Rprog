o. Configure Virtual Host
	-. copy pca.conf into /etc/apache2/sites-available/
		-. edit Port, DocumentRoot, Directory, LogLevel in pca.conf
	-. check /etc/apache2/ports.conf
		-. add port used by VirtualHost in pca.conf
o. Apply changes
	-. sudo a2ensite pca
	-. sudo service apache2 restart

