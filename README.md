Banjax
======

CircleCI status:
  * master [![CircleCI](https://circleci.com/gh/equalitie/banjax/tree/master.svg?style=svg&circle-token=86713a4ce3cf55d0c00b3ac62943f7fecd8a3e2a)](https://circleci.com/gh/equalitie/banjax/tree/master)
  * devel [![CircleCI](https://circleci.com/gh/equalitie/banjax/tree/devel.svg?style=svg&circle-token=86713a4ce3cf55d0c00b3ac62943f7fecd8a3e2a)](https://circleci.com/gh/equalitie/banjax/tree/devel)

Apache Traffic Server Plugin performing various anti-DDoS measures

Copyright 2017 eQualit.ie

Banjax is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see `<http://www.gnu.org/licenses/>`.

Installation
============

Installation from so file
-------------------------
Requirements:

Following standard Debian packages are required by banjax

     apt-get install libzmq libyaml-cpp-dev

Google re2 regex engine:

        git clone https://github.com/google/re2
        cd re2
        make test
        make install
        make testinstall

Copy banjax.so file in:

     /usr/local/trafficserver/modules

Copy banjax.conf file to the folder of your choice. The default directroy that banjax is going to look for banjax.conf is ATS default plugin directory, for example:

     /usr/local/trafficserver/modules/banjax/

Add a line

    banjax.so [/path/to/the/folder/where/banjax.conf/resides]

to

    /usr/local/trafficserver/conf/plugins.conf

If you omit the path arguement, banjax will look into ATS plugin directory for banjax.conf.

Installation from source
------------------------
Staging branch always contain the most recent semi stable source. Master branch is laging far behind the current development.

Banjax is using automake frame work, to make and install banjax, cd into banjax directory, assuming that the plugin directory of the traffic server is

    /usr/local/trafficserver/modules

You need the following dev deb package to compile banjax in addition to libre2 described above:

    apt-get install build-essential git libzmq-dev unzip automake libtool pkg-config libssl-dev libboost-dev

If you do not have ATS installed, you need to install it as well:

    apt-get install trafficserver-dev

notes that banjax require traffic server version 4 or higher and the build project might fails if you use earlier version provided by your distribution.

Clone banjax repo

    git clone https://github.com/equalitie/banjax

Move to the banjax folder

    cd <where you want banjax to be built>

invoke following sequences of command

    cmake <banjax directory>
    make
    cp libbanjax.a /usr/local/trafficserver/modules/

Testing:
--------
running unittests in banjax/test will run the unittests corresponding to banjax filters.

Behavior tests needs to be run as root. You also need to add the following line to conf/remap.conf

```
regex_map   http://^(www\.)?127.0.0.1$/ ORIGIN_URL
```

For example, if the http server is running on the same machine, listening on port 8000, you should add:

```
regex_map   http://^(www\.)?127.0.0.1$/ http://127.0.0.1:8000/
```

You should also need to have write access to the folder where http origin server websites from.

```
usage: banjax_behavior_test.py [-h] [--banjax-dir BANJAX_DIR]
                               [--banjax-test-dir BANJAX_TEST_DIR]
                               [--http-doc-root HTTP_DOC_ROOT]
optional arguments:
    -h, --help shows help
    --banjax-dir banjax config folder (default: /usr/local/trafficserver/modules/banjax/)
    --banjax-test-dir (default: current folder)
    --http-doc-root origin website folder (default: /var/www)
```


```
cd test
python banjax_behavior_test
```

Available Filters
=================
Currently 4 filters are implemented in Banjax. One can configure each filter behavior in banjax.conf. The configuration of each filter is accomplished via yaml, with a "challenger" and a "regex_banner" dictionary existing in the configuration. The "challenges" list under "challenger" contains the challenges to be proffered to users".

The order that each filter configuration appears in banjax.conf matters and  determine the order that banjax.conf run the filter. For example, there is no point to put white_lister filter at the end.

white_lister
============

White listed IPs do not go through any other filter configured with lower priority than white_lister. For example the ip address of  monitoring programs such as nagios needs to be white listed, so bot stoppers such as challenger does not prevent them from their duty. To white list an ip range, such as Google bots or a subnet use CIDR (IP/number of matching bits) notation.

White listed ips need to be added to the white_listed_ips array.
---------------
    white_lister:
      white_listed_ips:
        - 192.168.1.1
        - 10.0.0.1/24

regex_banner
============

regex_banner bans each request based on the specific rate they are matching regexes.

Each request consists of the following parts

METHOD uri host UA

such as

GET http://wiki/ equalit.ie "Firefox 1.0.1"

Currently the hit rate computed for each regex in the sense that it is the  number of time that a request matches each regexes, hence for example if a request matches two different regexes, it counts once as hit for each regexes.

To configure a new regex, you need to add a new rule in to the banned_regexes array. fields of the rule should be specified in yaml format (using colon)

rule: human readable explanation of what rule is about. It is *not* optional.
regex: a regex to match the whole request. Pay attention that you need to put \ when you are intending to have one \ in the regex. Also there are characters which do not get matched to "." so you need to use "[\s\S]" instead.

interval: The span of time in seconds when you want banjax keep record of the hit.

hits_per_interval: the number of hit per interval that results in banning

Banjax bans based on 1/1000*hit_per_interval per millisecond rate. Hence it does not wait for number of hits to reach hits_per_interval, as soon as an ip reach the rate of 1/1000*hit_per_interval per millisecond it will get banned.

If you want to ban a specific regex in first apperance you need to set hits_per_interval = 0

Sample Attacks:
---------------
* If the bot net is alway requesting "http://host.com/vmon" you can use the first rule.
* If a bot is requesting pages with rate of higher than 100 request per minute and you want to ban any IP with higher requests than that use the second rule.

---------------
    regex_banner :
      - rule: 'too much veggie monster'
        regex: '.*vmon[\\s\\S]*'
        interval: 1;
        hits_per_interval: 0
      - rule: dos
        regex: '[\\s\\S]*'
        interval: 60
        hits_per_interval: 100;

How does regex_banner works
---------------------------
Regex Banner uses the following method to keep the approximate rate of hit of each IP without storing every instance of hit.

Regex banner stores the hit rate in hit/millisecond and keep the rate for the given interval and the time stamp for the beginning of the interval. Once a new hit comes in:

If the beginning time stamp till the hit time stamps is larger than the interval then it will subtracts (time stamp - now)/interval* rate from the rate.

Independently it sums the current rate with 1/(interval*1000) (rate computed in times per millisecond).

Challenger
==========
Challenger serves different challenges to confirm the legitimacy of the client for the requested access (read, edit, etc). Currently partial inverse SHA256 , Captcha puzzles or password authentication are supported. The hash solution of the puzzle is also sent along side with the ip of the requester. It is mainly meant to be a cache busting prevention mechanism as well as cache-less access to the website.

key: is the string from which MAC key that is used to authenticate the cookie is being used. MAC prevents the attacker from tampering with the challenge or reuse its solution for different bots.

difficulty: it determines the number of leading zeros that the  SHA256(solution) at least should have in binary representation. Hence adding this value by 1 doubles the difficulty of the challenge.

For each host then the user needs to specifies the following parameters:

name: the host name as it appears in the url, "www.host.com" and "host.com" are treated as two different hosts and needs two separate config records.

validity_period: determine how often the challenger should re-challenge a client in seconds. When under cache-busting attack, it is advisable to decrease this number. Also because the solution is cookie based each client will be challenged once for each website.

challenge_type: can be "captcha", "sha_inverse" or "auth" at the moment. Note that currently banjax can only challenge each client using one challenge. Practically, this means that you should always define "auth" challenge before sha_inv or captcha challenge for a particular host.   

challenge: the name of the html file that contains the challenge. by default it is "captcha.html", "solver.html" and "auth.html", however user can copy these files (residing in modules/banjax/) to customize the appearance for example for localization.

no_of_fails_to_ban (optional): If specifies, challenger reports the ip to swabber, if the ip asks for the challenge this many times and fails to solve it (ip needs two request per captcha challenges).

password_hash (mandatory for auth): B64 encoded of SHA256 of the password that needs to be verified by challenger.

magic_word (mandatory for auth): the word that need to appear in the requested url in order for the challenger to serve the auth challenge token, to enable the client to generate the auth cookie.

Sample Attacks:
---------------
* If www.equalit.ie is under cache busting attack and you want to prevent the bots from reaching the origin you can use the first set of host rules, so challenger serves captcha before reaching to the origin. Note that you need a new set of host rule for equalit.ie if both www.equalit.ie and equalit.ie are resolving to your website.

* If wiki.deflect.ca is being attacked by a botnet that is not able to run Java Script or you would like to slow down each bot request by making them solve a problem before being served, at the same time you want to ban anybody who failed to solve 10 problems, you can use the second rule.

---------------
    challenger:
      key : "thisisakey"
      difficulty : 4 # Number of leading bits in the inverse sha challenge to be zero
      challenges:
        - name : "www.equalit.ie_captcha"
          domains:
           - "equalit.ie"
           - "www.equalit.ie
          challenge_type : "captcha"
          challenge : "captcha.html"
          no_of_fails_to_ban: 20
          validity_period : 86400
          
        - name : "wiki.deflect.ca_sha"
          challenge_type : "sha_inverse"
          challenge : "solver.html"
          no_of_fails_to_ban : 10
          validity_period : 120
          
         - name : "example.com_auth"
           domains:
             - "example.com"
             - "www.example.com"
           challenge_type : "auth"
           challenge : "auth.html"
           password_hash : "BdZitmLkeNx6Pq9vKn6027jMWmp63pJJowigedwEdzM="
           # This can either be a single entry or a list of entries
           # Any URL that matches this regular expression will be protected by authorization.
           magic_word : "iec1OoghAogh0ionieJaot4p"
           # As a list of entries
           #magic_word :
           #  - "foo"
           #  - "bar"
           validity_period : 1200
           no_of_fails_to_ban : 10


BotSniffer
==========
Bot sniffer reports the detail of each transaction to BotBanger to test the requester against a pre-learned model to see if the behavior of the ip resembles a bot. The data are encrypted using AES-GCM mode with subscription "botbanger_log" in clear text. The config is the zmq socket port where BotSniffer should publish the log into as well as the passphrase. The passphrase will be hashed by SHA256 to generate the encryption key for 32-byte AES-GCM. The encrypted token is organized as follows:

    12BytesIV|ENCRYPTEDDATE|16BytesAuthtoken

With | stand for concatination with no separator.

Sample Attacks:
---------------
* If you have Learn2ban model for your attack and Botbanger is running on your edge listening to port 22621 then you can add the following to banjax.conf to inform BotBanger about the requests to the edge

---------------
    bot_sniffer :
        botbanger_port: 22621
        key: "somebodysbeenworkingindark!"



Swabber
=======
This is a configuration entry for the Swabber interface. all fields are optional

    swabber:
      server: 127.0.0.1
      port: 22620
      grace_period: 600

server: is the ip of the interface which Swabber is listening on. default is "*".
port: is the port that swabber is listening on, default is "22620"
grace_period: swabber interface only report an ip to swabber only if: 1) ip is reported for banning by a filter and  "grace_period" seconds has passed since the first request for banning. if grace_period is set to 0 (when edges are too stressed) the ip is reported upon first banning request. default is 0.

Note that all other filters are still in effect next time the reported ip send a new request and so it means that the request does not reach the origin. However, challenger and auth still provide challenges. If it is desired that no filter get engaged with the reported ip Denialator filter should be activated.

Swabber interface records its log activities in

    trafficserver/logs/ban_ip_list.log

The general format of swabber interface log is as follows:

    [bot_ip], [time], [banning_reason], [flagged/banned]

Example:

    127.0.0.1, [2016-01-22T22:18:56], matched regex rule deflectsecret, GET, http:///secretpage, deflect.ca:8080, "Mozilla/5.0 (X11; Linux x86_64; rv:42.0) Gecko/20100101 conkeror/1.0pre1", flagged
    127.0.0.1, [2016-01-22T22:28:22], failed challenge deflectc-captcha for host deflect.ca:8080 10 times, "Mozilla/5.0 (X11; Linux x86_64; rv:42.0) Gecko/20100101 conkeror/1.0pre1" asking for http   :///__validate/, flagged
    127.0.0.1, [2016-01-22T22:28:46], flagged on 1453501702, grace period passed. reported by denialator, banned

flagged/banned:
  flagged: The IP is reported to swabber interface by a filter for the first time. it is not reported to swabber for banning.  until grace_period of the swabber is passed.
  banned: The grace_period of the swabber for this IP is passed since it was flagged first and as such it is reported to the swabber.

Note that the flagged line in the banjax log only appears once, the first time the ip is flagged. The all subsequent requests for banning does not appear in the log till it gets banned.

  banning_reason

Each filter send the reason by itself and not all filters have access to the same data, the banning filters log formats are as follows:

regex_manager:

    matched regex rule [rule_name], [METHOD], "[URL]", [HOST], "[UA]"

challenger:

    failed challenge [challenge name]  [number of failures] times, "[URL]", [HOST], "[UA]"

denialator:

    flagged on [flagged time in epoch], grace period passed. reported by denialator

notes that all internal double quotes (") for the "[URL]" and "[UA]" is escaped (replace by \")

Denialator
==========
To activate Denialator following line need to put in the config.

    denialator:

Upon activation denialator checks if the ip previously has been reported for banning and if such it denies access to the ip. The denialator will report the ip to swabber again once it the ip visits after that the grace period is finished. To disengage all filters from dealing with reported ip, denialator priority need to be set to 1 or 2.

How To Write A Filter
=====================

To add your own new filter to banjax you need to follow these steps:

1) Inherit from BanjaxFilter.
2) Add the constant representing your filter to the FilterIDType enum in filter_list.h e.g.
   WHITE_LISTER_FILTER_ID

3) Add the constant filter name in filter_list.h, e.g.:
   const std::string WHITE_LISTER_FILTER_NAME = "white_lister";

4) Write the constructor to set the filter name and id.

5) Override the load_config and execute to load the config of your filter and execute the operation your filter suppose to do. Override

6) Override requested_info to return the flags of all parts your filter need to analysis the request. for example:
  uint64_t requested_info() { return
      TransactionMuncher::IP     |
      TransactionMuncher::METHOD |
      TransactionMuncher::URL    |
      TransactionMuncher::HOST   |
      TransactionMuncher::UA;}

6) If it is needed override generate_response to generate your response instead of serving the request.

7) in Banjax::filter_factory add the new name to the loop

8) include the filter header file in banjax.cpp

9) add the cpp file to the src/Makefile.am

If your filter needs to keep state
----------------------------------
You need to ask banjax to allocate space for your filter in ip_db. To do so add your filter id to filter_to_column array in ip_database.h:

const FilterIDType filter_to_column[] = {
  REGEX_BANNER_FILTER_ID
  };

You'll get 16 bytes of memory for each ip, if you need more memory to keep the state you need to allocate it yourself and store a pointer to it in ip_database.

You need to edit the constructor of the filter to receive a pointer to the ip_database:

    RegexManager(const std::string& banjax_dir, const libconfig::Setting& main_root, IPDatabase* global_ip_database)

And you need to store it in ip_database member variable of BanjaxFilter (the parent of your filter) for further use. Finally you need edit

    Banjax::filter_factory

and tell banjax to send the pointer to your filter upon creation.

