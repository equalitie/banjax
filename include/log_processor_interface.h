 /*
 * Tools to initiate and communicate with the LogProcessor thread, Log processor
 * is responsible to run any filter that needs to process logs from multiple requests/
 * threads
 *
 * Copyright (c) eQualit.ie 2014 under GNU AGPL V3.0 or later
 * 
 * Vmon: January 2014
 */

#ifndef BANJAX_LOG_PROCESSOR_INTERFACE_H
#define BANJAX_LOG_PROCESSOR_INTERFACE_H

#include "log_entry_processor.h"
class BanjaxLogProcessorInterface
{
 protected:
  LogEntryProcessor *leProcessor;

 public:
    enum LOG_PROCESSOR_ERROR {
      CONFIG_ERROR
    };

  void SendLogEntryToLogProcessor(LogEntry *le) 
  {
    leProcessor->AddLogEntry(le); TSDebug("banjax", "sending to processor");
  }

  /**
     initiating the interface and the processor and set its behavoir according to
     configuration.
     
     @param cfg pointer to log_processor config of banjax.conf
  */
  BanjaxLogProcessorInterface(const libconfig::Setting& cfg);

  /**
     Destructor: stopping the thread, etc.
   */
  ~BanjaxLogProcessorInterface();
  
 };

#endif /*db_tools.h*/
/**
   Vmon-Kees conversation on the subject:

<vmon> I see so if we move these direct call inside the interface then it will                 be less confusing from the other side too                        [18:21]
<ks> that would become:
<ks> class BanjaxLogProcessorInteraction:public                                              LogEntryProcessorEventListener,public BotSnifferEventListener
<ks> {
<ks> }
<ks> I renamed LogEntryListener to BotSnifferEventListener to get the                        perspective right (BotSniffer generates the event)
<ks> the only thing that is tricky is that the LogEntryProcessor cleans up                   it's own events                                                    [18:22]
<vmon> I think we sholud keep it simple
<vmon> LogProcessorAction2Banjax calls a function in 
<vmon> the interface
<vmon> and then interface can call a function in banjax
<vmon> but                                                              [18:24]
<vmon> the lisnter is good enough 
<ks> LogProcessorAction2Banjax implements the LogEntryProcessorEventListener,                so this one works from the perspective of the LogEntryProcessor
<vmon> I just want to keep the interface as simple as possible          [18:25]
<vmon> so a programmer can read the code in the interface without knowning                     anything of logprocess event structure                           [18:26]
<vmon> so the interface isn't inheritedf from anything in the processor                 <ks> Ok, I would opt to move LogProcessorAction2Banjax to banjax
<ks> Create a BotSnifferEventListener instance which knows about 
<ks> These could live in the same file, being very small.
<ks> No, the interface for the LogEntryProcessor is :                   [18:28]
<ks> class LogEntryProcessorEventListener
<ks> {
<ks> public:
<ks>    virtual void OnLogEntryStart(LogEntry *le)=0;
<ks>    virtual void OnLogEntryEnd(LogEntry *le,string                                                                                                                                                             
     &output,vector<externalAction> &actionList)=0;
<ks>    virtual ~LogEntryProcessorEventListener() {;}
<ks> };
<vmon> I would vote for a unified interface
<ks> a unified implementation, not an interface I think                 [18:29]
<vmon> if you want to  know how log processor and banjax are interacting got                                                                                                                                       
       to this file
<ks> yep                                                                [18:30]
<vmon> Botsninfir -> interface -> processor
<ks> processor-> interface-> banjax
<vmon> yes but one class
<ks> one class 2 interfaces (abstract classes can be used as interfaces)
<vmon> and not inherited from LogEntryProcessorEventLisner              [18:31]
<vmon> bunch of functions that both side can call
<ks> it needs to inherit from LogEntryProcessorListener because that is where                                                                                                   
<ks> LogProcessor software only sees: LogEntryProcessorEventLisner      [18:32]     
<vmon> but that can happen in the processor then the listener in the processor                                                                                                                                     
       can call a function in th e interface
<ks> That would be an extra abstraction you just would be adding transparant                                                                                                                                       
     interfaces to each other                                           [18:33]
<vmon> yes but a programmer reading the interface doesn't need to know any                                                                                                                                         
       thingi LogEntryProcessorevintlistener                            [18:34]
<vmon> that level helps to hid e those stuff
<ks> In bot_sniffer he would'nt know
<ks> In the Interaction class you would want to know what is happening, so                                                                                                                                         
     this should implement both interfaces                              [18:36]
<ks> in bot_sniffer, he only would see a call to a BotSnifferEventListener                                                                                                                                         
     instance
<vmon> so we have ban_ip in LogProcessorInterface
<vmon> and sendEntry
<vmon> sendEntry is called by bot_sniffer                               [18:37]
<vmon> ban_ip calledb by
<vmon> LogProccssorBanjaxAction...
<ks> ok I see what you are getting at, ban_ip is called by what is now                                                                                                                                             
     LogProcessorAction2Banjax 
<vmon> if the programmer wants to know what is going behind the secen the thy                                                                                                                                      
       are welcome to go to logprocessor code                           [18:38]
<vmon> and dig more
<vmon> is just to capsulate anything dealing with logprossor            [18:40]
<ks> you want to keep the LogProcessorAction2Banjax seperate or merge them                                                                                                                                         
     with a BotSnifferEventListener interface? Or switch perspective and let                                                                                                                                       
     BotSniffer and LogProcessor2Banjax know about the LogProcessorInteraction
<vmon> I think the second one                                           [18:41]
<ks> Ok this means LogProcessor2Banjax will be mostly as is, but will talk to                                                                                                                                      
     a LogProcessorInteraction class instead of banjax                  [18:43]
<ks> and BotSniffer also talks to a LogProcessorInteraction class
<ks> LogProcessorInteraction will also create, configure etc....
<ks> This means we really should move LogProcessor2Banjax to /src
<ks> But we already wanted that                                         [18:44]
<vmon> I think
<vmon> we calll these classes intefaces rather than interaction
<vmon> though ;)
<vmon> and move the creation of log processor to there too              [18:45]
<vmon> so it is not all over the place
<ks> hmm, try to think of interface as java or c# interfaces, a defined                                                                                                                                            
     contract without implementation                                    [18:46]
<ks> But who, what when, you 've seen my other mail, my work is cut out for                                                                                                                                        
     me, can you move the LogEntryProcessor to the middle class which handles                                                                                                                                      
     the interaction/interfacing?                                       [18:48]
<vmon> you mean I should have written that in the email?                [18:51]
<ks> no
<ks> skip that, do you want the "middle class" to have an instance on the                                                                                                                                          
     Banjax class, so you can call it from BotSniffer?                  [18:52]
<vmon> I think I can move the  logentryprocessor stuff                  [18:53]
<vmon> if that is simpler
<vmon> then I'll ask you to verify if I did it correctly
<ks> ok that would be better as can begin around this time tomorrow with the                                                                                                                                       
     unit testing etc...
<ks> as can = as I can                                                  [18:54]
<ks> busy day tomorrow for me
     the Config to an instance which takes a Section
<ks> this should be the only change you need
<ks> in the src/processor tree                                          [18:58]
<ks> and maybe move LogProcessor2Banjax out
 */



