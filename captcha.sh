#!/bin/bash

CookieFileName=cookies.txt

res=$(curl -o /dev/null --silent --write-out '%{http_code}\n' http://127.0.0.1/)

if [ ! $res -eq 403 ] 
then
    echo "FAIL: test should return 403 when no cookie given"
    exit -1
fi

res=$(curl -o /dev/null --silent --write-out '%{http_code}\n' --cookie-jar $CookieFileName --cookie cookies.txt http://127.0.0.1/__captcha)

if [ ! $res -eq 200 ] 
then
    echo "FAIL: captcha should return 200 OK"
    exit -1
fi

res=$(curl -o /dev/null --silent --write-out '%{http_code}\n' --cookie-jar $CookieFileName --cookie cookies.txt  http://127.0.0.1/__validate/aaaaa)

if [ ! $res -eq 200 ] 
then
    echo "FAIL: /__validate/aaaaa should return 200 OK"
    exit -1
fi

res=$(curl -o /dev/null --silent --write-out '%{http_code}\n' --cookie-jar $CookieFileName --cookie cookies.txt http://127.0.0.1/)

if [[ $res -gt 400 || $res -lt 200 ]] 
then
    echo "FAIL: Expected to be authenticated, origin should now give us 200/OK: $res"
    exit -1
fi

echo "Captcha test OK"
rm $CookieFileName