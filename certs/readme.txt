Generate certificate for https
openssl req -newkey rsa:2048 -new -x509 -days 3652 -nodes -out server.crt -keyout server.pem
