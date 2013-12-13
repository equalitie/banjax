1. BUILDING

In the banjax directory run
autoreconf -i --force && ./configure
or
autoreconf -i --force && sh ./configure-debug.sh
to build banjax

1.1 GTEST

Gtest has been moved out of the project, the build now depends on a system install of the gtest library and headers instead of a local install
On a clean install with gtest installed we had a mismatch between the library and the local headers.

1.2 SVM

svm is included in source form (/src/processor/utils/svm), this will guarantee banjax will be building as svm is not a standard library. We might have to look into this for gtest as well.

LOGENTRY (src/processor/log_entry.h)

LogEntry is the struct we use for the request event, it has several limitations on the fields 
char hostname[100];
char userAgent[120];
char url[400]; // just cut off after 400
time_t endTime;
long msDuration;
int httpCode;
int payloadsize;
int requestDepth;
CacheLookupStatus cacheLookupStatus;// (hit/miss/error)
char useraddress[40]; 
char contenttype[80];


2 BOTBANGER

The BotBangerAggregator works by aggregating LogEntry entries and calculating the features.  
Also see 6.4 

2.1 CONFIGURATION 
bot_banger:{
	# maximum IPS to follow
	max_ips=60000; 
	# trace configuration
	trace_flags=0;  
	configurations=(
		{
			# this is the model used
			filename="../../../logfiles/analyse_2013-11-22 03#3a47#3a07.540226.normal_svm_model";
			# a name for the model 
			name="test1"; 
			actions=
				(					
					#multiple (overlapping) entries can be added					
					#value_lower,value_upper is always inclusive
					#the first match will be used 
					
					#between svm_predict 1..1 the action is "blockip,<ip>,test1"
					{value_lower=1.0;value_upper=1.0;action="blockip"} 					
				)	
		}
	)
};

Both the bot_banger and hitmiss configuration have a subsetting trace_flags which can enable the same tracing as done by the commandline version. The integer value is a combination of one or more of the following flags 
TraceHitMissAction=1,
TraceBotBangerAction=2,
TraceHitMissRatio=4,
TraceBotBangerFeatures=8,
TraceBotBangerModelValues=16,
TraceBotBangerModelInputs=32,
TraceLogEntries=64,
TraceBotBangerIPEvict=512
(see log_entry_processor.h) 

3 HOSTHITMISS

Host hit miss works by sampling a period in several ranges, this period and range (in seconds) can be configured by the banjax.conf.
The HostHitMissAggregator takes a logentry, looks this up in a map and passes this to a HostHitMissFeature for the The HostHitMissFeature has a circular buffer which can hold at most (period/range)+1 HitMissRange entries. When a new range starts the totals which are not in the current period are subtracted, this will keep the calculation performant.The ratio calculated is the hit/total. Every time after a LogEntry has been added the Aggregator, it will trigger the event listeners. 

HostHitMissActions is the listener implementation that will determine which action to take, it can be configured per host, req lower/upper limit range, ratio lower/upper range and the action to be taken.
Also see 6.4 

3.x CONFIGURATION


hitmiss:
{
	#this is the sliding window used to calculate the HIT ratio (HIT/TOTAL)  
	period=60; 
	# the sliding window is divided in parts of 5 seconds
	range=5; 
	# trace configuration
	trace_flags=0; 
	configurations=
	(
		# match the configuration to determine the action, lower/upper are always inclusive
		# the ratio is a double value so it needs a <.> (the configuration will be invalid if you use 0 instead of 0.0)
		# request_upper/lower will be matched to the number of requests in a period
		# ratio will be matched to the hit/total ratio
		{host="www.test.org";action="captcha";request_lower=1000;request_upper=999999;ratio_lower=0.99;ratio_upper=1.0;runtime=500},
		{host="www.test.org";action="captcha";request_lower=1000;request_upper=999999;ratio_lower=0.0;ratio_upper=0.3;runtime=500},
		{host="www.test.org";action="normal";request_lower=0;request_upper=999999;ratio_lower=0.0;ratio_upper=0.3;runtime=0}
		
	)	
};

See the botbanger section for the valid trace_flags

4 LOGPROCESSOR

4.1 STANDALONE COMMANDLINE VERSION
In src/processor LogProcessor will be created, this can create a trace of a 
logfile 
usage:./LogProcessor [options] [logfile]
options:
--logentries                     show logentry (time/ip/url)
--hitmissratio                   show hitmiss ratio
--hitmissaction                  show hitmiss action
--bbfeatures                     show botbanger features
--bbmodelinputs                  show botbanger normalized inputs
--bbmodelvalues                  show botbanger model predicted values
--bbaction                       show botbanger actions
--all                            show all
--config [filename]              read config file
--memoryusage                    show predicted memory usage (ballpark figure)

running a logfile will produce tabbed output, this can be read by excel or another program for further analysis. The configuration file can be a banjax configuration or a configuration with only the bot_banger and hitmiss subsections.  

4.2 INTEGRATION IN BANJAX

4.2.1 SENDING A LOGENTRY FROM BANJAX 

This is implemented in banjax.cpp, banjax.h and bot_sniffer.cpp, in banjax.cpp there is the setup and configuration of the LogEntryProcessor (using the configuration read from banjax.conf) 
In bot_sniffer.cpp a LogEntry struct is created and send into the LogProcessor via the Banjax object.

4.2.2 ACTING UPON A LOGENTRYPROCESSOR ACTION

This is implemented only by outputting the actions via TSDEBUG 
In /src/processor/log_processor_action_2_banjax.h there is a working stub which must be augmented with the functionality to 
trigger swabber and the challenger per host
-virtual void OnLogEntryEnd(LogEntry *le,string &output,vector<externalAction> &actionList)
In the actionList you can expect the actions which are
<action>,ip,modelname    #for any botbanger configuration
<action>,hostname        #for any hosthitmiss configuration

4.2.3 TRACING THE VALUES

The banjax version can output the same tracing as the commandline version
if the configuration trace_flags is added to the bot_banger and hitmiss 
configuration entries.


5 IMPROVEMENTS/THINGS TO CHECK

The HostHitMissAggregator now has no limits on the number hostnames it monitors, it creates the map dynamicaly. This makes it vulnerable to a random host attack. 
This can be fixed by pre-filling the host map with configured values. We loose flexibility then though.  

The FeaturePercentageConsecutiveRequests feature uses the whole path now to remember the last request, this could/should be done with a sha1 hash value, this would save 380 bytes per IP.

Starting and configuring the LogEntryProcessor might be moved to a seperate banjax filter instead of the banjax main . We haven't done this now because the LogProcessor is conceptually something different. It does not participate in a normal HTTP request flow and does not need continuations to work.

HostHitMiss and BotBanger are not persistent right now, the features calculated will not survive a traffic_server restart.

If a website is truly load balanced it might be desirable to move botbanger processing to a place where it can read aggregated logs. It might be that a svm model is robust enough to act upon a partial trace of an IP (VMON?), then this will not be necessary. 

For normalization we now use the incremental utils/std_dev.h, this is not as efficient as it could be, but still fast enough to have no impact on system 
performance.

6 OTHER THINGS

6.1 ECLIPSE

As this is build in a eclipse environment there are .cproject files added to 
this branch, if you don't need them, delete them.

6.2 MEMORYUSAGE
The --memoryusage commandline option will give the expected maximum memory usage, this is a ballpark figure, not taking into account alignment and some overheads.  

6.3 ARCHITECTURE
The LogEntryProcessor runs in a seperate thread. LogEntries are fed by a concurrent fifo queue. 
We have made two assumptions:
-The LogEntryProcessor is fast enough to prevent the queue from filling up and eating up the memory. if this is likely to be a problem we could:
	- streamlining the LogEntry, making it smaller, we do not need the whole url, we can precompute depth and use a sha1 of the url to let the features work
	- shedding LogEntries if the queue becomes too big
-Lock contention won't be a problem, the chance of two requests being in the same call to the concurrent queue is negligible.

6.4 GENERAL CALLING HIERARCHY

6.4.1 Command line processor:
-log_processor_main.cpp/main
	-read command line setup
	-log_entry_processor.cpp/LogEntryProcessor/constructor
	-Setup processor (6.4.6)
	-log_entry_processor.cpp/LogEntryProcessor/Start (start processor thread 6.4.5)
	-read entries (loop) 
		-log_processor_main.cpp/ParseLogLine
		-log_entry_processor.cpp/LogEntryProcessor/AddLogEntry ->pushes LogEntry to queue (processed by 6.4.5)
	-

6.4.2 Banjax setup
-banjax.cpp/TSPluginInit
	-Banjax/Constructor
		-Create log_processor
		-Banjax/read_configuration
		-Setup log processor with configuration read (6.4.6)
		-register event listener log_processor_action_2_banjax/LogProcessorAction2Banjax (also see 6.4.4)
	-log_entry_processor.cpp/LogEntryProcessor/Start (start processor thread 6.4.5)
	

6.4.3 Banjax log_entry generation and processing
-bot_sniffer.cpp/BotSniffer/execute
	-create LogEntry
	-log_entry_processor.cpp/LogEntryProcessor/AddLogEntry ->pushes LogEntry to queue (processed by 6.4.4)

6.4.4 Banjax action integration
This is called when 6.4.5 has processed a logentry, this cannot have heavy processing or blocking IO, as this will fill up the concurrent queue. 
-LogProcessorAction2Banjax/OnLogEntryEnd 
	- write all gathered tracing output to TSDebug
	- write all gathered actions to TSDebug
	- call Banjax/BanIP for blockip action (should be a rare action)
	- !!!! still need an integration point for the captcha action (should be a rare action), enable captcha processing for host

6.4.5 LogEntry processing
-log_entry_processor.cpp/LogEntryProcessor/Start (async=true)
	-start thread 
		-processorThread
			-innerProcesserThread
				-Loop on ReceiveLogEntry till an end message is received (1 byte)
					-AggregrateLogEntry
						-call LogEntryProcessor eventlisteners/OnLogEntryStart
						-HostHitMissAggretator/Aggregate (6.4.7)
						-BotBangerAggregator/Aggregate (6.4.8)
						-call LogEntryProcessor eventlisteners/OnLogEntryEnd					

6.4.6 Setup processor
-log_entry_processor_config.cpp/LogEntryProcessorConfig/ReadFromSettings 
	-read configuration file
	-setup BotBanger and HostHitMiss global configuration (period/range/max_ips)
	-depending on configuration create listeners and register them with the processor
		-hosthitmiss/host_hit_miss_dumper.h/HostHitMissDumper
		-HostHitMissActionDumper (a logging instance of the HostHitMissActionCollector)
		-BotBangerFeatureDumper (dumps the features)
		-BotBangerValueDumper (a logging instance of the BotBangerActionCollector)
		-LogEntryProcessorDumper (in console mode this will output the output to stdout)
		-HostHitMissActionCollector 
		-BotBangerActionCollector 
	-read the subconfiguration for HostHitMiss/Botbanger and setup the HostHitMissActionCollector and BotBangerActionCollector

6.4.7 HostHitMissAggretator/Aggregate
-add host and reserve computing space if necessary
-no pruning!!
-HostHitMissFeature/Aggregrate
	- move to new range, and subtract old ranges if currenttime out of current range
	- calculate range hit/total ratio and period hit/total ratio
-Call eventlisteners/OnHostHitMissEvent
	- HostHitMissActions has all the configuration stuff for the actions (HostHitMissActionDumper/HostHitMissActionCollector  inherit from this class)
	- Other classes registered in 6.4.6 will serve tracing 

6.4.8 BotBangerAggregator/Aggregate 
-add IP address and reserve computing space for features if necessary
-prune 10% of the entries if we hit max_ips, we can do this because we are singlethreaded
-clear featurecontainer if max idle time has been reached
-FeatureContainer/Aggregrate
	-loop all features
		-Feature/Aggregrate
			-cast feature data structure if necessary
			-recalc feature
-call OnFeatureEvent for all registered listeners
	-BotBangerModelListener has all the model calculation and configuration stuff (BotBangerActionCollector/BotBangerValueDumper inherit from this class)
	-Other classes registered in 6.4.6 will serve tracing


