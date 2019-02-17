# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#
PROJECT_NAME := secure-mqtt-client

include $(IDF_PATH)/make/project.mk

credentials:
	openssl genrsa -out main/client.key 2048
	openssl req -out main/client.csr -key main/client.key -new -subj "/C=CA/ST=BC/L=Kelowna/O=client.taylorsoftware.ca/OU=client/CN=maggie/emailAddress=wtaylor@taylorsoftware.ca"
	openssl x509 -req -in main/client.csr -CA ~/data/projects/mqtt/OpenWRT/mosq_ca.crt -CAkey ~/data/projects/mqtt/OpenWRT/mosq_ca.key -CAcreateserial -out main/client.crt -days 365
	openssl x509 -in main/client.crt -noout -text
