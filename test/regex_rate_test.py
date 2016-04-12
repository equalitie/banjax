#!/usr/bin/env python

import urllib2
from time import sleep

# test that we are not authorized initially
print "test that the desired rate gets banned"
req = urllib2.Request("http://127.0.0.1:8080/firstrate")  
res = None
status = []
expected_status = [404]*3 + [403]
times = 4
total_interval = 35.0
rate = times/total_interval
for i in range(0, times):
    sleep(rate)
    try:
        res = urllib2.urlopen(req)
        status.append(res.getcode())
    except urllib2.HTTPError, error:
        status.append(error.getcode())
        pass
   
assert status == expected_status, "Expected" + str(expected_status) + "but got status " + str(status)
# print "fetch captcha and cookie"
# req = urllib2.Request("http://127.0.0.1/__captcha")  
# res = None
# status = -1
# res = urllib2.urlopen(req) 
# status = res.getcode()
# cookie = res.info()['Set-Cookie'][len("deflect="):][:-1*len("; path=/; HttpOnly")]

# assert status == 200, "Captcha should return 200: %d" % status
# assert len(cookie) > 0, "Captcha should return a cookie: %d" % status


# print "validate"
# req = urllib2.Request("http://127.0.0.1/__validate/aaaaa", headers={"Cookie" : "deflect=%s" %  cookie})  
# res = None
# status = -1
# res = urllib2.urlopen(req) 
# status = res.getcode()
# cookie = res.info()['Set-Cookie'][len("deflect="):][:-1*len("; path=/; HttpOnly")]
# assert status == 200, "validate should return 200: %d" % status
# assert len(cookie) > 0, "validate should return a cookie: %d" % status

# print "ensure we are authorized with the cookie we got from the validator"
# req = urllib2.Request("http://127.0.0.1", headers={"Cookie" : "deflect=%s" %  cookie})  
# res = None
# status = -1
# res = urllib2.urlopen(req) 
# status = res.getcode()
# cookie = res.info()['Set-Cookie'][len("deflect="):][:-1*len("; path=/; HttpOnly")]
# assert status >= 200 and status < 400, "After validation, we should not receive an error code" % status
