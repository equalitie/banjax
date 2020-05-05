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
from queue import Queue
from threading import Thread

from http.server import BaseHTTPRequestHandler, HTTPServer

import pexpect
import asyncio
import random
import string

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
        self.wfile.write(self.server.body.encode())

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

    KAFKA_CHALLENGE_CONFIG = (
       "priority:\n"
       "  white_lister: 1\n"
       "  challenger: 2\n"
        "\n"
       "white_lister:\n"
       "    white_listed_ips:\n"
       "      - 1.1.1.1\n"
        "\n"
       "challenger:\n"
       "    difficulty: 0\n"
       "    key: 'allwearesayingisgivewarachance'\n"
       "    challenges: []\n"
        "\n"
       "kafka:\n"
       "  brokers: 'localhost:9092'\n"
       "  failed_challenge_topic: 'failed_challenge_ips'\n"
       "  challenge_host_topic: 'hosts_to_challenge'\n"
       "  status_topic: 'banjax_statuses'\n"
       "  dynamic_challenger_config:\n"
       "    name: 'from-kafka-challenge'\n"
       "    challenge_type: 'sha_inverse'\n"
       "    challenge: 'solver.html'\n"
       "    magic_word:\n"
       "      - ['regexp', '.*']\n"
       "    validity_period: 360000  # how long a cookie stays valid for\n"
       "    white_listed_ips:        # XXX i needed this for some reason\n"
       "      - '0.0.0.0'\n"
       "    no_of_fails_to_ban: 2    # XXX think about what this should be...\n");


    def print_debug(self):
        opts = { "standard": self.ats_bin_dir() +  "/../var/log/trafficserver/traffic.out"
               , "apache":   self.ats_prefix_dir + "/logs/traffic.out" }

        subprocess.Popen(["tail", "-f", opts[Test.ats_layout]]);

    # Auxilary functions
    def read_page(self, page_filename):
        with open(self.banjax_test_dir() + "/"+ page_filename, 'rb') as page_file:
            return page_file.read().decode()

    def read_solver_body(self):
        with open(self.banjax_module_dir() + "/solver.html") as solver_body:
            self.SOLVER_PAGE_PREFIX = solver_body.read(self.SOLVER_PAGE_PREFIX_LEN)

    def restart_traffic_server(self):
        with subprocess.Popen([self.ats_bin_dir() + "/trafficserver", "restart"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE) as traffic_proc:
            traffic_proc.wait()
            time.sleep(Test.ATS_INIT_DELAY)
            self.assertEqual(traffic_proc.stderr.read(), b"")
            return traffic_proc.stdout.read()

    def stop_traffic_server(self):
        with subprocess.Popen([self.ats_bin_dir() + "/trafficserver", "stop"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE) as traffic_proc:
            traffic_proc.wait()

    def clear_cache(self):
        self.stop_traffic_server();
        with subprocess.Popen([self.ats_bin_dir() + "/traffic_server", "-K"],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE) as proc:
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
        with subprocess.Popen(curl_cmd,
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE) as curl_proc:

            return curl_proc.stdout.read().decode()

    def set_banjax_config(self, name, value):
        # When using TS 6.0
        #subprocess.call([self.ats_bin_dir() + "/traffic_line", "-s", name, "-v", value])
        # When using TS 7.0
        subprocess.call([self.ats_bin_dir() + "/traffic_ctl", "config", "set", name, value])

    def setUp(self):
        print("setUp: ", self._testMethodName)
        self.read_solver_body()
        self.server = Server()

    def tearDown(self):
        self.stop_traffic_server()
        self.server.stop()
        print("tearDown: ", self._testMethodName)

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

    def test_white_list_per_host(self):
        """
        White listed IPs don't need to go through authorization when
        on a page marked with MAGIC_WORD.
        """
        config = ("priority:\n"
                  "  white_lister: 1\n"
                  "  challenger: 2\n"
                  "\n"
                  "white_lister:\n"
                  "    white_listed_ips:\n"
                  "      - host: "+Test.ATS_HOST+"\n"
                  "        ip_range: 127.0.0.1\n"
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
                  "        validity_period: 120\n"
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

    def test_bypass_subnet_range_per_host(self):
        """
        Same as the above 'test_bypass_ips_per_host' test, but instead of the
        white listed IP use an IP range.
        """
        config = (
            "challenger:\n"
            "    difficulty: 30\n"
            "    key: 'allwearesayingisgivewarachance'\n"
            "    challenges:\n"
            "      - name: 'example.co_auth'\n"
            "        domains:\n"
            "         - '"+Test.ATS_HOST+"'\n"
            "        white_listed_ips:\n"
            "         - 127.0.0.0/24\n"
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
            "priority:\n"
            "    challenger: 1\n"
            "    white_lister: 2\n"
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
            "priority:\n"
            "    challenger: 1\n"
            "    white_lister: 2\n"
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

    def test_magic_words_regexp_and_substr(self):
        config = (
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
            "        magic_word:\n"
            "          - 'wp-login0.php'\n"
            "          - ['substr', 'wp-login1.php']\n"
            "          - ['regexp', 'wp-login2.php']\n"
            "        validity_period: 120\n");

        self.replace_config2(config)

        # Tell TS to disable caching
        self.set_banjax_config("proxy.config.http.cache.http", "0")

        def get(page):
            return self.do_curl(Test.ATS_HOST + "/" + page)

        def expect_secret(page):
            self.server.body = "secret"
            result = get(page)
            self.assertEqual(result[:Test.COMP_LEN], self.read_page(Test.AUTH_PAGE)[:Test.COMP_LEN]);

        def expect_public(page):
            self.server.body = "not secret"
            result = get(page)
            self.assertEqual(result, self.server.body);

        expect_public('foobar')

        expect_secret('wp-login0.php')
        expect_public('wp-login0-php')

        expect_secret('wp-login1.php')
        expect_public('wp-login1-php')

        expect_secret('wp-login2.php')
        expect_secret('wp-login2-php')

    def test_magic_word_exceptions(self):
        config = (
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
            "        magic_word:\n"
            "          - 'protected'\n"
            "        magic_word_exceptions:\n"
            "          - exception\n"
            "        validity_period: 120\n");

        self.replace_config2(config)

        # Tell TS to disable caching
        self.set_banjax_config("proxy.config.http.cache.http", "0")

        def get(page):
            return self.do_curl(Test.ATS_HOST + "/" + page)

        def expect_secret(page):
            self.server.body = "secret"
            result = get(page)
            self.assertEqual(result[:Test.COMP_LEN], self.read_page(Test.AUTH_PAGE)[:Test.COMP_LEN]);

        def expect_public(page):
            self.server.body = "not secret"
            result = get(page)
            self.assertEqual(result, self.server.body);

        expect_public('foobar')

        expect_secret('protected')
        expect_public('protected/exception')
        expect_secret('protected?foo=exception')

    def test_kafka_stuff(self):
        loop = asyncio.get_event_loop()
        loop.run_until_complete(async_main(self))


#################
# Kafka Stuff
# i thought using async/await with pexpect would make the itegration with the kafka
# producer + consuemr scripts nicer to work with.
#################


def random_string(length):
    return ''.join(random.choice(string.ascii_lowercase) for i in range(length))


all_children = []

async def wait_for(child, pattern):
    i = await child.expect(
        [
            pattern,
            pexpect.TIMEOUT,
            pexpect.EOF
        ],
        timeout=60,
        async_=True
    )
    if i is not 0:
        print("command %s did not see pattern %s before timeout" % (' '.join(child.args), pattern))
        return False
    print("command %s DID see pattern %s before timeout" % (' '.join(child.args), pattern))
    return True

def start(command):
    child = pexpect.spawn(command, maxread=1)
    all_children.append(child)
    return child

async def async_main(self):
    kafka_dir = "./kafka_2.12-2.5.0/"
    topic = "hosts_to_challenge"
    test_message = random_string(10)
    self.replace_config2(self.KAFKA_CHALLENGE_CONFIG)
    try:
        zookeeper_p = child1 = start("{kafka_dir}bin/zookeeper-server-start.sh {kafka_dir}config/zookeeper.properties".format(kafka_dir=kafka_dir))
        assert await wait_for(zookeeper_p, r'.*binding to port.*')

        kafka_p = start("{kafka_dir}bin/kafka-server-start.sh {kafka_dir}config/server.properties".format(kafka_dir=kafka_dir))
        assert await wait_for(kafka_p, r'.*started \(kafka.server.KafkaServer.*')

        create_topic_p = start("{kafka_dir}bin/kafka-topics.sh --create --bootstrap-server localhost:9092 --replication-factor 1 --partitions 1 --topic {topic}".format(kafka_dir=kafka_dir, topic=topic))
        await create_topic_p.expect(pexpect.EOF, async_=True)
        print("after create")

        consumer_p = start("{kafka_dir}bin/kafka-console-consumer.sh --bootstrap-server localhost:9092 --topic {topic} --from-beginning".format(kafka_dir=kafka_dir, topic=topic))
        print("after consumer")

        producer_p = start("{kafka_dir}bin/kafka-console-producer.sh --broker-list localhost:9092 --topic {topic}".format(kafka_dir=kafka_dir, topic=topic))
        print("after producer")

        producer_p.sendline(test_message)
        assert await wait_for(consumer_p, test_message)
        print("got test message")

        # assert no challenger
        self.server.body = "some-string1"
        result = self.do_curl(Test.ATS_HOST + "/some-route")
        self.assertEqual(result, "some-string1");

        # send "turn challenger on" message
        # assert yes challenger
        # XXX FML kafka is slow and this can take tens of seconds?!?
        # really should be waiting for the log output from traffic.out
        producer_p.sendline(Test.ATS_HOST)
        assert await wait_for(consumer_p, Test.ATS_HOST)
        await asyncio.sleep(30)
        result = self.do_curl(Test.ATS_HOST + "/some-route")
        self.assertTrue("Please turn on JavaScript and reload the page" in result)


        # wait for timeout
        # assert no challenger
        await asyncio.sleep(60)
        result = self.do_curl(Test.ATS_HOST + "/some-route")
        self.assertEqual(result, "some-string1");


    finally:
        for child in all_children:
            child.close()

#################
# End Kafka Stuff
#################


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
