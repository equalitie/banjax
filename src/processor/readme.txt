1. BUILDING

In the banjax directory run
autoreconf -i --force && ./configure
or
autoreconf -i --force && sh ./configure-debug.sh
to build banjax

1.1 GTEST

Gtest has been moved out of the project, the build now depends on a system 
install of the gtest library and headers instead of a local install, on a clean
install with gtest installed we had a mismatch between the library and the local 
headers

1.2 SVM

svm is included in source form (/src/processor/utils/svm), this will guarantee
banjax will be building as svm is not a standard library, we might have to look 
into this for gtest as well

LOGENTRY (src/processor/log_entry.h)

LogEntry is the struct we use for the request event, it has several limitations
on the fields 
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
2.1 FEATURES
2.1.1 FEATURECONTAINER
2.1.2
2.2 CONFIGURATION 
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

Both the bot_banger and hitmiss configuration have a subsetting trace_flags which can enable 
the same tracing as done by the commandline version. The integer value is a combination   
of one or more of the following flags 
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

running a logfile will produce tabbed output, this can be read by excel or 
another program for further analysis. The configuration file can be a banjax 
configuration or a configuration with only the bot_banger and hitmiss subsections.  

4.2 INTEGRATION IN BANJAX

4.2.1 SENDING A LOGENTRY FROM BANJAX 

This is implemented in banjax.cpp, banjax.h and bot_sniffer.cpp, in banjax.cpp
there is the setup and configuration of the LogEntryProcessor (using the 
configuration read from banjax.conf) 
In bot_sniffer.cpp a LogEntry struct is created and send into the LogProcessor 
via the Banjax object.

4.2.2 ACTING UPON A LOGENTRYPROCESSOR ACTION

This is implemented only by outputting the actions via TSDEBUG 
In /src/processor/log_processor_action_2_banjax.h
there is a working stub which must be augmented with the functionality to 
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

The HostHitMissAggregator now has no limits on the number hostnames it 
monitors, it creates the map dynamicaly. This makes it vulnerable to a
random host attack. 
This can be fixed by pre-filling the host map with configured values. 
We loose flexibility then though.  

The FeaturePercentageConsecutiveRequests feature uses the whole path now to
remember the last request, this could/should be done with a sha1 hash value,
this would save 380 bytes per IP.

Starting and configuring the LogEntryProcessor might be moved to a seperate 
banjax filter instead of the banjax main . We haven't done this now 
because the LogProcessor is conceptually something different. It does not 
participate in a normal HTTP request flow and does not need continuations to work.

HostHitMiss and BotBanger are not persistent right now, the features calculated
will not survive a traffic_server restart.

If a website is truly load balanced it might be desirable to move botbanger 
processing to a place where it can read aggregated logs. It might be that
a svm model is robust enough to act upon a partial trace of an IP (VMON?),
then this will not be necessary. 

For normalization we now use the incremental utils/std_dev.h, this is not
as efficient as it could be, but still fast enough to have no impact on system 
performance.

6 OTHER THINGS

6.1 ECLIPSE

As this is build in a eclipse environment there are .cproject files added to 
this branch, if you don't need them, delete them.
