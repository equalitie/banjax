# Copyright 2011, 2012 SRI International
# See LICENSE for other credits and copying information

# Copyright 2013 eQualit.ie under AGPL v3.0 or higher

# AUTHORS: Vmon: Initial version adopted from Stegotorus

import os
import os.path
import shutil
import subprocess
import sys #for argv
import time

import unittest
from Queue import Queue
from threading import Thread

from pdb import set_trace as tr

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer

class S(BaseHTTPRequestHandler):
    def do_GET(self):
        self.handle_()

    def do_HEAD(self):
        self.handle_()

    def do_POST(self):
        self.handle_()

    def handle_(self):
        self.send_response(self.server.response_code)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        self.wfile.write(self.server.body)

class Server(HTTPServer):
    def __init__(self, body = "not set"):
        self.body = body
        self.response_code = 200

        HTTPServer.__init__(self, ('', 8000), S)

        def run(s):
            s.serve_forever()

        self.thread = Thread(target=run, args=(self,))
        self.thread.daemon = True
        self.thread.start()
        time.sleep(0.1)

    def stop(self):
        self.shutdown()
        self.socket.close()
        self.thread.join()

class DeadManSwitch:
    def __init__(self, time_to_wait):
        def run(q, time_to_wait):
            # Throws on timeout
            q.get(True, time_to_wait)

        self.queue = Queue()
        self.thread = Thread(target=run, args=(self.queue,time_to_wait,))
        self.thread.daemon = True
        self.thread.start()

    def stop(self):
        self.queue.put(1)
        self.thread.join()

class Test(unittest.TestCase):
    BANNED_URL = "127.0.0.1/vmon"
    BANNED_MESSAGE = "<html><header></header><body>Forbidden</body></html>"
    ATS_HOST = "127.0.0.1:8080"

    MAGIC_WORD      = "iloveyoumoinonplus"
    AUTH_COOKIE     = "deflect=m/FW19g3DzzDxDrC2eM+Zn+KsZP5fAmRAAAAAA==bNfg7zBaZHmqSwiLtvKpqzi4QFt/bknzYOmjjbaPhe8="
    BAD_AUTH_COOKIE = "deflect=DeeiSdkg/fu1w5hnq0p9V1A/fXawj5/TAAAAAAuwxyzabcdefghijklmnopqrstuvwxyz3uffRiGLQ60s="
    AUTH_PAGE       = "auth.html"
    CACHED_PAGE     = "cached_page.html"
    UNCACHED_PAGE   = "uncached_page.html"

    SOLVER_PAGE_PREFIX_LEN = 100;
    COMP_LEN = 100;

    ATS_INIT_DELAY = 6

    ats_layout     = "apache" # or "standard"
    ats_prefix_dir = ""
    ats_bin_dir    = "/usr/local/trafficserver/bin"

    def banjax_module_dir(self):
        options = { "standard": Test.ats_prefix_dir + "/libexec/trafficserver/banjax"
                  , "apache":   Test.ats_prefix_dir + "/modules/banjax" }

        return options[Test.ats_layout]

    def ats_bin_dir(self):
        options = { "standard": Test.ats_prefix_dir + "/bin"
                  , "apache":   Test.ats_prefix_dir + "/bin" }

        return options[Test.ats_layout]

    def banjax_test_dir(self):
        return os.path.dirname(os.path.realpath(__file__))

    AUTH_CHALLENGE_CONFIG = (
        "challenger:\n"
        "    difficulty: 0\n"
        "    key: 'allwearesayingisgivewarachance'\n"
        "    challenges:\n"
        "      - name: 'example.co_auth'\n"
        "        domains:\n"
        "         - '"+ATS_HOST+"'\n"
        "        challenge_type: 'auth'\n"
        "        challenge: '"+AUTH_PAGE+"'\n"
        "        # sha256('howisbabbyformed?')\n"
        "        password_hash: 'BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM='\n"
        "        magic_word: '"+MAGIC_WORD+"'\n"
        "        magic_word_exceptions: ['wp-admin/admin.ajax.php']\n"
        "        validity_period: 120\n"
        "        no_of_fails_to_ban: 10\n");

    def print_debug(self):
        opts = { "standard": self.ats_bin_dir() +  "/../var/log/trafficserver/traffic.out"
               , "apache":   self.ats_prefix_dir + "/logs/traffic.out" }

        subprocess.Popen(["tail", "-f", opts[Test.ats_layout]]);

    # Auxilary functions
    def read_page(self, page_filename):
        page_file  = open(self.banjax_test_dir() + "/"+ page_filename, 'rb')
        return page_file.read()

    def read_solver_body(self):
        solver_body = open(self.banjax_module_dir() + "/solver.html")
        self.SOLVER_PAGE_PREFIX = solver_body.read(self.SOLVER_PAGE_PREFIX_LEN)

    def restart_traffic_server(self):
        traffic_proc = subprocess.Popen([self.ats_bin_dir() + "/trafficserver", "restart"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        traffic_proc.wait()
        time.sleep(Test.ATS_INIT_DELAY)
        self.assertEqual(traffic_proc.stderr.read(), "")
        return traffic_proc.stdout.read()

    def stop_traffic_server(self):
        traffic_proc = subprocess.Popen([self.ats_bin_dir() + "/trafficserver", "stop"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        traffic_proc.wait()

    def clear_cache(self):
        self.stop_traffic_server();
        proc = subprocess.Popen([self.ats_bin_dir() + "/traffic_server", "-K"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        proc.kill()
        proc.wait()

    def replace_config2(self, config_string):
        self.clear_cache()
        config_path = self.banjax_module_dir() + "/banjax.conf"
        config = open(config_path, 'w')
        config.write(config_string)
        config.close()
        #We need to restart ATS to make banjax to read the config again
        return self.restart_traffic_server()

    def do_curl(self, url, cookie = None):
        curl_cmd = ["curl", url] + (cookie and ["--cookie", cookie] or [])
        curl_proc = subprocess.Popen(curl_cmd,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)

        return curl_proc.stdout.read()

    def set_banjax_config(self, name, value):
        # When using TS 6.0
        #subprocess.call([self.ats_bin_dir() + "/traffic_line", "-s", name, "-v", value])
        # When using TS 7.0
        subprocess.call([self.ats_bin_dir() + "/traffic_ctl", "config", "set", name, value])

    def setUp(self):
        print "setUp: ", self._testMethodName
        self.read_solver_body()
        self.server = Server()

    def tearDown(self):
        self.stop_traffic_server()
        self.server.stop()
        print "tearDown: ", self._testMethodName

    def ntest_request_banned_url(self):
        self.replace_config("banned_url_test.conf")
        result = self.do_curl(self.Test.BANNED_URL)
        self.assertEqual(result,self.BANNED_MESSAGE);

    def ntest_unbanned_challenged_url(self):
        self.replace_config("challenged_url_test.conf")
        result = self.do_curl(ALLOWED_URL)
        self.assertEqual(result[0:self.SOLVER_PAGE_PREFIX_LEN],self.SOLVER_PAGE_PREFIX);

    def ntest_unchallenged_white_listed_ip(self):
        tr()
        self.replace_config("white_listed.conf")
        result = self.do_curl(Test.BANNED_URL)
        self.assertEqual(result,self.ALLOWED_PAGE);

    def test_auth_challenged_success(self):
        """
        This test simulate a correct password entered in the auth page
        after entring the password it checks that ATS serves directly
        from origin everytime
        """
        self.replace_config2(self.AUTH_CHALLENGE_CONFIG)

        # Tell TS to start caching.
        self.set_banjax_config("proxy.config.http.cache.http", "1")
        self.set_banjax_config("proxy.config.http.cache.required_headers", "0")

        time.sleep(5)

        #Auth page
        self.server.body = "page0"
        result = self.do_curl(Test.ATS_HOST + "/" +Test.MAGIC_WORD)
        self.assertEqual(result[:Test.COMP_LEN], self.read_page(Test.AUTH_PAGE)[:Test.COMP_LEN]);

        #request to guarantee cache if fails
        self.server.body = "page1"
        result = self.do_curl(Test.ATS_HOST + "/" + Test.CACHED_PAGE, cookie = Test.AUTH_COOKIE)
        self.assertEqual(result, self.server.body)

        #check if it is not cached
        self.server.body = "page2"
        result = self.do_curl(Test.ATS_HOST + "/" + Test.CACHED_PAGE, cookie = Test.AUTH_COOKIE)
        self.assertEqual(result, self.server.body);

        #check that the nocache reply is not cached
        self.server.body = "page3"
        result = self.do_curl(Test.ATS_HOST + "/" + Test.CACHED_PAGE, cookie = Test.AUTH_COOKIE)
        self.assertEqual(result, self.server.body);

    def test_auth_challenged_failure(self):
        """
        The test simulate entering a wrong password in the auth challenge
        a banned message is expected to be served
        """
        # TODO: This test sometimes fails because the first time a page is downloaded
        #       it is already cached. So figure out how to clear the cache on the
        #       traffic server before the below code is executed.
        #       NOTE: TS v7.2 this can be done with `traffic_ctl server --clear-cache`
        #             but I haven't yet found a way how to do it with previous versions.
        page       = Test.CACHED_PAGE
        host       = Test.ATS_HOST
        bad_cookie = Test.BAD_AUTH_COOKIE

        self.replace_config2(self.AUTH_CHALLENGE_CONFIG)

        #request to guarantee cache
        self.server.body = "body0"
        result = self.do_curl(host + "/" + page, cookie = bad_cookie)
        self.assertEqual(result, "body0")

        #check if it is reading from cache if the cookie is bad
        self.server.body = "body 1"
        result = self.do_curl(host + "/" + page, cookie = bad_cookie)
        self.assertEqual(result, "body0")

    def test_auth_challenged_unchallenged_cached(self):
        """
        This test request a website with auth challenge but it does
        not invoke the magic word, hence ATS should serve the page
        through cache constantly
        """
        self.replace_config2(self.AUTH_CHALLENGE_CONFIG)

        self.server.body = "page1"
        result = self.do_curl(Test.ATS_HOST + "/" + Test.CACHED_PAGE)
        self.assertEqual(result, self.server.body)

        self.server.body = "page2"
        result = self.do_curl(Test.ATS_HOST + "/" + Test.CACHED_PAGE)
        self.assertEqual(result, "page1");

    def test_problem_with_origin(self):
        """
        In case of non standard errors from the origin, the traffic server
        hanged for ~one minute. This was unacceptable as users thought there
        was actually a problem with the traffic server. So it was fixed.
        """
        self.replace_config2(self.AUTH_CHALLENGE_CONFIG)

        # This is a non standard HTTP message VMon was apparently seening
        # when creating the issue https://redmine.equalit.ie/issues/2089
        ERR_CONNECT_FAIL = 110

        self.server.response_code = ERR_CONNECT_FAIL

        dms = DeadManSwitch(5)
        result = self.do_curl(Test.ATS_HOST + "/" + Test.CACHED_PAGE)
        # TODO: Test we really receive the error code.
        dms.stop()

    def test_white_listed_magic_must_not_be_cached(self):
        """
        White listed IPs don't need to go through authorization when
        on a page marked with MAGIC_WORD. We must make sure that such
        pages don't go to cache otherwise other white listed users
        may see them.
        """
        config = ("priority:\n"
                  "  white_lister: 1\n"
                  "  challenger: 2\n"
                  "\n"
                  "white_lister:\n"
                  "    white_listed_ips:\n"
                  "      - 127.0.0.1\n"
                  "\n")

        config += self.AUTH_CHALLENGE_CONFIG

        self.replace_config2(config)

        # Tell TS to start caching.
        self.set_banjax_config("proxy.config.http.cache.http", "1")
        self.set_banjax_config("proxy.config.http.cache.required_headers", "0")

        def get(page):
            return self.do_curl(Test.ATS_HOST + "/" + page)

        self.server.body = "page0"
        result = get(Test.MAGIC_WORD)
        self.assertEqual(result, self.server.body)

        self.server.body = "page1"
        result = get(Test.MAGIC_WORD)
        self.assertEqual(result, self.server.body)

    def test_bypass_ips_per_host(self):
        config = (
            "challenger:\n"
            "    difficulty: 30\n"
            "    key: 'allwearesayingisgivewarachance'\n"
            "    challenges:\n"
            "      - name: 'example.co_auth'\n"
            "        domains:\n"
            "         - '"+Test.ATS_HOST+"'\n"
            "        white_listed_ips:\n"
            "         - 127.0.0.1\n"
            "        challenge_type: 'auth'\n"
            "        challenge: '"+Test.AUTH_PAGE+"'\n"
            "        # sha256('howisbabbyformed?')\n"
            "        password_hash: 'BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM='\n"
            "        magic_word: '"+Test.MAGIC_WORD+"'\n"
            "        validity_period: 120\n");

        self.replace_config2(config)

        # Tell TS to start caching.
        self.set_banjax_config("proxy.config.http.cache.http", "1")
        self.set_banjax_config("proxy.config.http.cache.required_headers", "0")

        def get(page):
            return self.do_curl(Test.ATS_HOST + "/" + page)

        # Accessing the secret page must work without a valid cookie because
        # We're white listed.
        self.server.body = "secret page"
        result = get(Test.MAGIC_WORD)
        self.assertEqual(result, self.server.body)

        # Make sure it is not cached.
        self.server.body = "secret page new"
        result = get(Test.MAGIC_WORD)
        self.assertEqual(result, self.server.body)

    def test_global_white_list_skip_inv_sha_challenge(self):
        #self.print_debug();

        config = (
            "white_lister:\n"
            "    white_listed_ips:\n"
            "      - 127.0.0.1\n"
            "challenger:\n"
            "    difficulty: 30\n"
            "    key: 'allwearesayingisgivewarachance'\n"
            "    challenges:\n"
            "      - name: 'example.co_auth'\n"
            "        domains:\n"
            "         - '"+Test.ATS_HOST+"'\n"
            "        challenge_type: 'sha_inverse'\n"
            "        challenge: '"+Test.AUTH_PAGE+"'\n"
            "        # sha256('howisbabbyformed?')\n"
            "        password_hash: 'BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM='\n"
            "        magic_word: '"+Test.MAGIC_WORD+"'\n"
            "        validity_period: 120\n");

        self.replace_config2(config)

        # Tell TS to disable caching
        self.set_banjax_config("proxy.config.http.cache.http", "0")

        def get(page):
            return self.do_curl(Test.ATS_HOST + "/" + page)

        # Accessing the secret page must work without a valid cookie because
        # We're white listed.
        self.server.body = "secret page"
        result = get(Test.MAGIC_WORD)
        self.assertEqual(result, self.server.body);

    def test_global_white_list_wont_skip_auth(self):
        config = (
            "white_lister:\n"
            "    white_listed_ips:\n"
            "      - 127.0.0.1\n"
            "challenger:\n"
            "    difficulty: 30\n"
            "    key: 'allwearesayingisgivewarachance'\n"
            "    challenges:\n"
            "      - name: 'example.co_auth'\n"
            "        domains:\n"
            "         - '"+Test.ATS_HOST+"'\n"
            "        challenge_type: 'auth'\n"
            "        challenge: '"+Test.AUTH_PAGE+"'\n"
            "        # sha256('howisbabbyformed?')\n"
            "        password_hash: 'BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM='\n"
            "        magic_word: '"+Test.MAGIC_WORD+"'\n"
            "        validity_period: 120\n");

        self.replace_config2(config)

        # Tell TS to disable caching
        self.set_banjax_config("proxy.config.http.cache.http", "0")

        def get(page):
            return self.do_curl(Test.ATS_HOST + "/" + page)

        # Accessing the secret page must work without a valid cookie because
        # We're white listed.
        self.server.body = "secret page"
        result = get(Test.MAGIC_WORD)
        self.assertEqual(result[:Test.COMP_LEN], self.read_page(Test.AUTH_PAGE)[:Test.COMP_LEN]);


if __name__ == '__main__':
    from unittest import main
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--ts-layout', default=Test.ats_layout)
    parser.add_argument('--ts-prefix', default=Test.ats_prefix_dir)
    parser.add_argument('unittest_args', nargs='*')

    args = parser.parse_args()
    Test.ats_layout     = args.ts_layout
    Test.ats_prefix_dir = args.ts_prefix

    # Now set the sys.argv to the unittest_args (leaving sys.argv[0] alone)
    sys.argv[1:] = args.unittest_args
    unittest.main()
    main()
