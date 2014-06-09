#!/usr/bin/env python

import urllib2

# test that we are not authorized initially
print "test that we are not authorized"
req = urllib2.Request("http://127.0.0.1/")  
res = None
status = -1
try:

    res = urllib2.urlopen(req) 
except urllib2.HTTPError, error:
    status = error.getcode()
    pass
   
assert status == 403, "Test should return 403 without a cookie"



print "fetch captcha and cookie"
req = urllib2.Request("http://127.0.0.1/__captcha")  
res = None
status = -1
res = urllib2.urlopen(req) 
status = res.getcode()
cookie = res.info()['Set-Cookie'][len("deflect="):][:-1*len("; path=/; HttpOnly")]

assert status == 200, "Captcha should return 200: %d" % status
assert len(cookie) > 0, "Captcha should return a cookie: %d" % status


print "validate"
req = urllib2.Request("http://127.0.0.1/__validate/aaaaa", headers={"Cookie" : "deflect=%s" %  cookie})  
res = None
status = -1
res = urllib2.urlopen(req) 
status = res.getcode()
cookie = res.info()['Set-Cookie'][len("deflect="):][:-1*len("; path=/; HttpOnly")]
assert status == 200, "validate should return 200: %d" % status
assert len(cookie) > 0, "validate should return a cookie: %d" % status

print "ensure we are authorized with the cookie we got from the validator"
req = urllib2.Request("http://127.0.0.1", headers={"Cookie" : "deflect=%s" %  cookie})  
res = None
status = -1
res = urllib2.urlopen(req) 
status = res.getcode()
cookie = res.info()['Set-Cookie'][len("deflect="):][:-1*len("; path=/; HttpOnly")]
assert status >= 200 and status < 400, "After validation, we should not receive an error code" % status
