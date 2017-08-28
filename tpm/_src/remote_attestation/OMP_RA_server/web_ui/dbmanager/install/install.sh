echo "Django Web Server install script"
echo "Edit path data in config files" 

pip install -r requirements.txt 

echo "mkdir -p /etc/init"
mkdir -p /etc/init
echo "mkdir -p /etc/uwsgi/vassals"
mkdir -p /etc/uwsgi/vassals
echo "mkdir -p /var/www/html/"
mkdir -p /var/www/html/

echo "cp sp_django.conf /etc/nginx/conf.d/"
cp sp_django.conf  /etc/nginx/conf.d/
echo "rm /etc/nginx/conf.d/default.conf"
rm /etc/nginx/conf.d/default.conf 
echo "cp sp_django.ini /etc/uwsgi/"
cp sp_django.ini /etc/uwsgi/
echo "cp uwsgi.conf /etc/init/"
cp uwsgi.conf /etc/init/
echo "cp -r /home/suser/remote_attestation_crypto_lib/_src/remote_attestation/OMP_RA_server/web_ui/dbmanager/raserver/item/static /var/www/html/"
cp -r /home/suser/remote_attestation_crypto_lib/_src/remote_attestation/OMP_RA_server/web_ui/dbmanager/raserver/item/static /var/www/html/
echo "ln -s /usr/local/lib/python2.7/site-packages/django/contrib/admin/static/admin /var/www/html/static/"
ln -s /usr/local/lib/python2.7/site-packages/django/contrib/admin/static/admin /var/www/html/static/
echo "ln -s /usr/local/lib/libpcre.so.1 /lib64"
ln -s /usr/local/lib/libpcre.so.1 /lib64

echo "cp sites.py /usr/local/lib/python2.7/site-packages/django/contrib/admin/sites.py"
cp sites.py /usr/local/lib/python2.7/site-packages/django/contrib/admin/sites.py
