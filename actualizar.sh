#!/bin/bash/
fecha=$(date +"%m-%d-%Y-%T")
cp -R /home/usuario/control /var/www/respaldo/Respaldo$fecha
service apache2 stop
service apache2 star
