#!/bin/bash

CGIMOD_S=/etc/apache2/mods-enabled/cgi.load
CGIMOD_R=/etc/apache2/mods-available/cgi.load

if [ -h $CGIMOD_S ]; then
	REALPATH=`/bin/readlink -nf $CGIMOD_S`
	if [ $REALPATH -ef $CGIMOD_R ]; then
		echo "CGI Enabled"
		exit 0 # OK
	fi
fi

echo "run sudo a2enmod cgi"

