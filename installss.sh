#!/bin/bash

yum install python-setuptools && easy_install pip

pip install shadowsocks

yum install vim

touch /etc/shadowsocks.json

echo "Please enter your ip address:"
read ip_addr

echo "Please set the password:"
read ip_passwd
echo '{
	"server": "$ip_addr",
	"server_port": 8388,
	"local_address": "127.0.0.1",
	"local_port": 1080,
	"password": "$ip_passwd",
	"timeout": 300,
	"method": "aes-256-cfb",
	"fast_open": false
}'

echo "starting sservice"

ssserver -c /etc/shadowsocks.json -d start

echo "start succeeded"
echo "You can use this to login ssserver"
echo "IP: $ip_addr"
echo "Password: $ip_passwd"
