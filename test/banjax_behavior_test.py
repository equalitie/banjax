# Copyright 2011, 2012 SRI International
# See LICENSE for other credits and copying information

# Copyright 2013 eQualit.ie under AGPL v3.0 or higher

# AUTHORS: Vmon: Initial version adopted from Stegotorus

import os
import os.path
import shutil
import subprocess
import sys #for argv

import unittest

from pdb import set_trace as tr

class BanjaxBehavoirTest(unittest.TestCase):

    BANNED_URL = "localhost/vmon"
    BANNED_MESSAGE = "<html><header></header><body>Forbidden</body></html>"
    ATS_HOST = "127.0.0.1"

    MAGIC_WORD = "iloveyoumoinonplus"
    AUTH_COOKIE = "deflect=DeeiSdkg/fu1w5hnq0p9V1A/fXawj5/TAAAAAAuRHVwMK26GySv0PJmDDd7QccMDiJSU/3uffRiGLQ60s="
    BAD_AUTH_COOKIE = "deflect=DeeiSdkg/fu1w5hnq0p9V1A/fXawj5/TAAAAAAuwxyzabcdefghijklmnopqrstuvwxyz3uffRiGLQ60s="
    AUTH_PAGE = "auth.html"
    CACHED_PAGE = "cached_page.html"
    UNCACHED_PAGE = "uncached_page.html"

    STD_OUT = 0
    STD_ERR = 1

    SOLVER_PAGE_PREFIX_LEN = 100;
    COMP_LEN = 100;

    ATS_INIT_DELAY = 5

    banjax_dir = "/usr/local/trafficserver/modules/banjax"
    banjax_test_dir = "/root/dev/banjax/test"
    ats_bin_dir = "/usr/local/trafficserver/bin"
    http_doc_root = "/var/www"

    # def __init__(self, banjax_dir = "/usr/local/trafficserver/modules/banjax"):
    #     """
    #     Set the banjax directory in the ats module directory. this is where the
    #     config file lives which needs to be backed up before testing

    #     INPUT::
    #        banjax_dir: the directory that contains banjax file. it is
    #                    /usr/local/trafficserver/modules/banjax
    #                    on a typical deflect edge
    #     """
    #     BanjaxBehavoirTest.banjax_dir = banjax_dir
    #     unittest.TestCase.__init__(self)

    #Auxilary functions
    def read_page(self, page_filename):
        page_file  = open(BanjaxBehavoirTest.banjax_test_dir + "/"+ page_filename, 'rb')
        return page_file.read()

    def read_solver_body(self):
        solver_body = open(BanjaxBehavoirTest.banjax_dir + "/solver.html")
        self.SOLVER_PAGE_PREFIX = solver_body.read(self.SOLVER_PAGE_PREFIX_LEN)

    def try_to_recover_backup_config(self):
        if (os.path.exists(BanjaxBehavoirTest.banjax_dir + "/banjax.conf.tmpbak")):
            shutil.copyfile(BanjaxBehavoirTest.banjax_dir + "/banjax.conf.tmpbak", BanjaxBehavoirTest.banjax_dir + "/banjax.conf")

    def check_config_exists(self):
        return os.path.exists(BanjaxBehavoirTest.banjax_dir + "/banjax.conf") and True or False

    def backup_config(self):
        shutil.copyfile(BanjaxBehavoirTest.banjax_dir + "/banjax.conf", BanjaxBehavoirTest.banjax_dir + "/banjax.conf.tmpbak")
        return os.path.exists(BanjaxBehavoirTest.banjax_dir + "/banjax.conf.tmpbak")

    def replace_config(self, new_config_filename):
        shutil.copyfile(self.banjax_test_dir + "/" + new_config_filename , BanjaxBehavoirTest.banjax_dir + "/banjax.conf")
        #We need to restart ATS to make banjax to read the config again
        traffic_proc = subprocess.Popen([BanjaxBehavoirTest.ats_bin_dir + "/trafficserver", "restart"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        traffic_proc.wait()
        import time
        time.sleep(BanjaxBehavoirTest.ATS_INIT_DELAY)
        return (traffic_proc.stdout.read(), traffic_proc.stderr.read())
        return ('','')

    def put_page(self, page_filename):
        shutil.copyfile(BanjaxBehavoirTest.banjax_test_dir + "/" + page_filename, BanjaxBehavoirTest.http_doc_root + "/" +page_filename)

    def replace_page(self, page_old, page_new):
        shutil.copyfile(BanjaxBehavoirTest.banjax_test_dir + "/" + page_new, BanjaxBehavoirTest.http_doc_root + "/" +page_old)

    def remove_page(self, page_filename):
        shutil.remove(BanjaxBehavoirTest.http_doc_root + "/" + page_filename)

    def do_curl(self, url, cookie= None):
        curl_cmd = ["curl", url] + (cookie and ["--cookie", cookie] or [])
        curl_proc = subprocess.Popen(curl_cmd,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        return (curl_proc.stdout.read(), curl_proc.stderr.read())

    def setUp(self):
        #check if the user has provided us with the config directory of
        #ATS or we should use the default
        self.assertEqual(self.check_config_exists(),True)

        self.read_solver_body()
        self.try_to_recover_backup_config()
        self.assertEqual(self.backup_config(), True)

    def tearDown(self):
        self.try_to_recover_backup_config()

    # def doTest(self, label, st_args):
    #     st = Stegotorus(st_args)
    #     tester = Tltester(self.scriptFile,
    #                       ("127.0.0.1:4999", "127.0.0.1:5001"))
    #     errors = ""
    #     try:
    #         testtl = tester.check_completion(label + " tester")
    #         if testtl != self.reftl:
    #             errors += diff("errors in transfer:", self.reftl, testtl)

    #     except AssertionError, e:
    #         errors += e.message
    #     except Exception, e:
    #         errors += repr(e)

    #     errors += st.check_completion(label + " proxy", errors != "")

    #     if errors != "":
    #         self.fail("\n" + errors)

    def ntest_request_banned_url(self):
        result = self.replace_config("banned_url_test.conf")
        self.assertEqual(result[self.STD_ERR], "")
        result = self.do_curl(self.BanjaxBehavoirTest.BANNED_URL)
        self.assertEqual(result[self.STD_OUT],self.BANNED_MESSAGE);

    def ntest_unbanned_challenged_url(self):
        self.replace_config("challenged_url_test.conf")
        result = self.do_curl(ALLOWED_URL)
        self.assertEqual(result[self.STD_OUT][0:self.SOLVER_PAGE_PREFIX_LEN],self.SOLVER_PAGE_PREFIX);

    def ntest_unchallenged_white_listed_ip(self):
        tr()
        self.replace_config("white_listed.conf")
        result = self.do_curl(BanjaxBehavoirTest.BANNED_URL)
        self.assertEqual(result[self.STD_OUT],self.ALLOWED_PAGE);

    def test_auth_challenged_success(self):
        """
        This test simulate a correct password entered in the auth page
        after entring the password it checks that ATS serves directly
        from origin everytime
        """
        #Auth page
        result = self.replace_config("auth_challenge_test.conf")
        self.assertEqual(result[BanjaxBehavoirTest.STD_ERR], "")
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" +BanjaxBehavoirTest.MAGIC_WORD)
        self.assertEqual(result[BanjaxBehavoirTest.STD_OUT][:BanjaxBehavoirTest.COMP_LEN],self.read_page(BanjaxBehavoirTest.AUTH_PAGE)[:BanjaxBehavoirTest.COMP_LEN]);

        #request to guarantee cache if fails
        self.put_page(BanjaxBehavoirTest.CACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE, cookie = BanjaxBehavoirTest.AUTH_COOKIE)
        self.assertEqual(result[self.STD_OUT], self.read_page(BanjaxBehavoirTest.CACHED_PAGE))

        #check if it is not cached
        self.replace_page(BanjaxBehavoirTest.CACHED_PAGE,BanjaxBehavoirTest.UNCACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE, cookie = BanjaxBehavoirTest.AUTH_COOKIE)
        self.assertEqual(result[self.STD_OUT],self.read_page(BanjaxBehavoirTest.UNCACHED_PAGE));

        #check that the nocache reply is not cached
        self.replace_page(BanjaxBehavoirTest.UNCACHED_PAGE,BanjaxBehavoirTest.CACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE, cookie = BanjaxBehavoirTest.AUTH_COOKIE)
        self.assertEqual(result[self.STD_OUT],self.read_page(BanjaxBehavoirTest.CACHED_PAGE));



    def test_auth_challenged_failure(self):
        """
        The test simulate entering a wrong password in the auth challenge
        a banned message is expected to be served
        """
        result = self.replace_config("auth_challenge_test.conf")
        self.assertEqual(result[BanjaxBehavoirTest.STD_ERR], "")
        #request to guarantee cache
        self.put_page(BanjaxBehavoirTest.CACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE, cookie = BanjaxBehavoirTest.BAD_AUTH_COOKIE)
        self.assertEqual(result[self.STD_OUT], self.read_page(BanjaxBehavoirTest.CACHED_PAGE))

        #check if it is reading from cache if the cookie is bad
        self.replace_page(BanjaxBehavoirTest.CACHED_PAGE,BanjaxBehavoirTest.UNCACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE, cookie = BanjaxBehavoirTest.BAD_AUTH_COOKIE)
        self.assertEqual(result[self.STD_OUT],self.read_page(BanjaxBehavoirTest.CACHED_PAGE));

    def test_auth_challenged_unchallenged_cached(self):
        """
        This test request a website with auth challenge but it does
        not invoke the magic word, hence ATS should serve the page
        through cache consitantly
        """
        result = self.replace_config("auth_challenge_test.conf")
        self.assertEqual(result[BanjaxBehavoirTest.STD_ERR], "")
        self.put_page(BanjaxBehavoirTest.CACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE)
        self.assertEqual(result[self.STD_OUT], self.read_page(BanjaxBehavoirTest.CACHED_PAGE))
        self.replace_page(BanjaxBehavoirTest.CACHED_PAGE,BanjaxBehavoirTest.UNCACHED_PAGE)
        result = self.do_curl(BanjaxBehavoirTest.ATS_HOST + "/" + BanjaxBehavoirTest.CACHED_PAGE)
        self.assertEqual(result[self.STD_OUT],self.read_page(BanjaxBehavoirTest.CACHED_PAGE));

# Synthesize TimelineTest+TestCase subclasses for every 'tl_*' file in
# the test directory.

if __name__ == '__main__':
    from unittest import main
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--banjax-dir', default=BanjaxBehavoirTest.banjax_dir)
    parser.add_argument('--banjax-test-dir', default=os.getcwd())
    parser.add_argument('--ats-bin-dir', default=BanjaxBehavoirTest.ats_bin_dir)
    parser.add_argument('--http-doc-root', default=BanjaxBehavoirTest.http_doc_root)
    parser.add_argument('unittest_args', nargs='*')

    args = parser.parse_args()
    BanjaxBehavoirTest.banjax_dir = args.banjax_dir
    BanjaxBehavoirTest.banjax_test_dir = args.banjax_test_dir
    BanjaxBehavoirTest.http_doc_root = args.http_doc_root
    BanjaxBehavoirTest.ats_bin_dir = args.ats_bin_dir

    traffic_proc = subprocess.Popen([BanjaxBehavoirTest.ats_bin_dir + "/trafficserver", "start"],
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
    traffic_proc.wait()

    print traffic_proc.stdout.read()
    std_err = traffic_proc.stderr.read()
    print std_err and "Error: \n"+std_err or ''
    # TODO: Go do something with args.input and args.filename

    # Now set the sys.argv to the unittest_args (leaving sys.argv[0] alone)
    sys.argv[1:] = args.unittest_args
    unittest.main()
    main()
