#openssl genrsa -des3 -out server.orig.key 2048
#openssl rsa -in server.orig.key -out server.key
#openssl req -new -key server.key -out server.csr
#openssl x509 -req -days 365 -in server.csr -signkey server.key -out server.crt

#openssl req -new -x509 -keyout server~.pem -out server~.pem -days 365 -nodes

openssl genrsa -out server~.key 4096
openssl req -new -key server~.key -out server~.csr -subj '/C=US/ST=NY/L=New York/CN=www.test.com'
openssl x509 -req -days 365 -in server~.csr -signkey server~.key -out server~.crt
