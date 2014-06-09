# Copyright 2011, 2012 SRI International
# See LICENSE for other credits and copying information

# Copyright 2013 eQualit.ie under AGPL v3.0 or higher

# AUTHORS: Vmon: Initial version adopted from Stegotorus

import os
import os.path
import shutil
import subprocess

import unittest

from pdb import set_trace as tr

class BanjaxBehavoirTest(unittest.TestCase):

    BANNED_URL = "localhost/vmon"
    BANNED_MESSAGE = "<html><header></header><body>Forbidden</body></html>"

    STD_IN = 0
    STD_OUT = 1
    STD_ERR = 2

    SOLVER_PAGE_PREFIX_LEN = 100;

    banjax_dir = "/home/vmon/doc/code/deflect/ats_lab/banjax/config"
    banjax_test_dir = "/home/vmon/doc/code/deflect/ats_lab/banjax/test"

    # def __init__(self, banjax_dir = "/usr/local/trafficserver/modules/banjax"):
    #     """
    #     Set the banjax directory in the ats module directory. this is where the
    #     config file lives which needs to be backed up before testing

    #     INPUT::
    #        banjax_dir: the directory that contains banjax file. it is
    #                    /usr/local/trafficserver/modules/banjax
    #                    on a typical deflect edge
    #     """
    #     self.banjax_dir = banjax_dir
    #     unittest.TestCase.__init__(self)

    #Auxilary functions
    def read_solver_body(self):
        solver_body = open(self.banjax_dir + "/solver.html")
        self.SOLVER_PAGE_PREFIX = solver_body.read(self.SOLVER_PAGE_PREFIX_LEN)

    def try_to_recover_backup_config(self):
        if (os.path.exists(self.banjax_dir + "/banjax.conf.tmpbak")):
            shutil.copyfile(self.banjax_dir + "/banjax.conf.tmpbak", self.banjax_dir + "/banjax.conf")

    def check_config_exists(self):
        return os.path.exists(self.banjax_dir + "/banjax.conf") and True or False

    def backup_config(self):
        shutil.copyfile(self.banjax_dir + "/banjax.conf", self.banjax_dir + "/banjax.conf.tmpbak")
        return os.path.exists(self.banjax_dir + "/banjax.conf.tmpbak")

    def replace_config(self, new_config_filename):
        shutil.copyfile(self.banjax_test_dir + "/" + new_config_filename , self.banjax_dir + "/banjax.conf")


    def do_curl(self, url):
        tr()

        curl_proc = subprocess.Popen(["curl", url],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        return (curl_proc.stdout.read(), curl_proc.stderr.read())

    def setUp(self):
        self.read_solver_body()
        self.try_to_recover_backup_config()
        self.assertEqual(self.check_config_exists(), True)
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

    def test_request_banned_url(self):
        tr()
        self.replace_config("banned_url_test.conf")
        result = self.do_curl(self.BANNED_URL)
        self.assertEqual(result[self.STD_OUT],self.BANNED_MESSAGE);

    def test_unbanned_challenged_url(self):
        tr()
        self.replace_config("challenged_url_test.conf")
        result = self.do_curl(ALLOWED_URL)
        self.assertEqual(result[self.STD_OUT][0:self.SOLVER_PAGE_PREFIX_LEN],self.SOLVER_PAGE_PREFIX);

    def test_unchallenged_white_listed_ip(self):
        tr()
        self.replace_config("white_listed.conf")
        result = self.do_curl(BANNED_URL)
        self.assertEqual(result[self.STD_OUT],self.ALLOWED_PAGE);

    def test_auth_challenged_success(self):
        """
        This test simulate a correct password entered in the auth page
        after entring the password it checks that ATS serves directly
        from origin everytime
        """
        tr()
        self.replace_config("auth_challenger.conf")
        result = self.do_curl(MAGIC_URL)
        self.assertEqual(result[self.STD_OUT],self.ALLOWED_PAGE);
        result = self.do_curl(NO_CACHE_PAGE_URL, cookie = ALLOWED_COOKIE)
        self.assertEqual(result[self.STD_OUT],self.CACHED_PAGE);
        self.replace_page(NO_CACHE_PAGE_FILE, result[self.STD_OUT],self.UN_CACHED_PAGE)
        self.assertEqual(result[self.STD_OUT],self.UN_CACHED_PAGE);

    def test_auth_challenged_failure(self):
        """
        The test simulate entering a wrong password in the auth challenge
        a banned message is expected to be served
        """
        tr()
        self.replace_config("auth_challenger.conf")
        result = self.do_curl(MAGIC_URL)
        self.assertEqual(result[self.STD_OUT],self.ALLOWED_PAGE);
        result = self.do_curl(NO_CACHE_PAGE_URL, cookie = BAD_PASS_COOKIE)
        self.assertEqual(result[self.STD_OUT],self.MANNED_MESSAGE);

    def test_auth_challenged_unchallenged_cached(self):
        """
        This test request a website with auth challenge but it does
        not invoke the magic word, hence ATS should serve the page
        through cache consitantly
        """
        tr()
        self.replace_config("auth_challenger.conf")
        result = self.do_curl(NO_CACHE_PAGE_URL)
        self.assertEqual(result[self.STD_OUT],self.CACHED_PAGE);
        self.replace_page(NO_CACHE_PAGE_FILE, result[self.STD_OUT],self.UN_CACHED_PAGE)
        self.assertEqual(result[self.STD_OUT],self.CACHED_PAGE);

# Synthesize TimelineTest+TestCase subclasses for every 'tl_*' file in
# the test directory.

if __name__ == '__main__':
    from unittest import main
    main()
