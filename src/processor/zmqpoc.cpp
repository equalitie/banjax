// zmqpoc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <zmq.h>
#include <stdio.h>

#include <string.h>
#include <assert.h>
#define _CRT_RAND_S 1

#include <stdlib.h>
#include "LogEntry.h"

#include "HostHitMissAggregator.h"
#include "FeatureAverageTimeBetweenRequests.h"
#include "FeatureHTTPStatusRatio.h"
#include "FeatureAveragePayloadSize.h"
#include "FeatureHtmlToImageRatio.h"
#include "FeatureRequestDepth.h"
#include "FeaturePercentageConsecutiveRequests.h"

#include "BotBangerAggregrator.h"
#include "BotBangerLogger.h"

#define TCPX

#ifdef TCP
#define socketlisten "tcp://*:5555"
#define socketdef "tcp://localhost:5555"
#else
#define socketlisten "inproc://#1"
#define socketdef "inproc://#1"
#endif
void *context;
DWORD WINAPI startServer(void *x)
{
	//  Socket to talk to clients
     
    void *responder = zmq_socket (context,ZMQ_PULL);
	
	int rc = zmq_bind (responder, socketlisten);
    assert (rc == 0);
	int recv=0;
    while (1) {
        char buffer [10000];
        zmq_recv (responder, buffer, 10000, 0);
		recv++;
		if (recv%10000==0)
			printf("%d\n",recv);
        //printf ("Received Hello %d\n",(int) x);
        //Sleep (1000);          //  Do some 'work'
        //zmq_send (responder, "World", 5, 0);
    }
    return 0;
	
}




DWORD WINAPI startClient(void *x)
{
	printf ("Connecting to hello world server�\n");
    
    void *requester = zmq_socket (context, ZMQ_PUSH);
	zmq_connect (requester, socketdef);

    int request_nbr;
	DWORD curtick=GetTickCount();
	char buffer [10000];
    for (request_nbr = 0; request_nbr != 10000000; request_nbr++) {
        
        //printf ("Sending Hello %d\n", request_nbr);
        zmq_send (requester, buffer, sizeof(LogEntry), 0);
		
        //zmq_recv (requester, buffer, 10, 0);
        //printf ("Received World %d\n", request_nbr);
    }

    zmq_close (requester);
    printf ("Ready %d\n",GetTickCount()-curtick);
    return 0;

}

int _xtmain(int argc, _TCHAR* argv[])
{
	/*context=zmq_ctx_new ();
	DWORD threadid;
	HANDLE threads[3];
	threads[0]=CreateThread(NULL,0,startServer,(void *) 2,0,&threadid);	
	threads[1]=CreateThread(NULL,0,startClient,NULL,0,&threadid);
	WaitForMultipleObjects(2,threads,true,INFINITE);
	zmq_ctx_destroy(context);*/


	HostHitMissAggregator *hm=new HostHitMissAggregator();
	BotBangerAggregator *bb=new BotBangerAggregator();
	//hm->RegisterEventListener(new HostHitMissLogger());
	bb->RegisterEventListener(new BotBangerLogger());
	

	bb->RegisterFeature(new FeatureAverageTimeBetweenRequests(),0);
	//bb->RegisterFeature(new FeatureCyclingUserAgent(),2);
	
	bb->RegisterFeature(new FeatureHtmlToImageRatio(),2);	
	bb->RegisterFeature(new FeatureAveragePayloadSize,4);
	bb->RegisterFeature(new FeatureHTTPStatusRatio(),5);
	bb->RegisterFeature(new FeatureRequestDepth(),6);
	bb->RegisterFeature(new FeaturePercentageConsecutiveRequests(),9);
	
	float myTime=0;
	float nextCheck=0;
	LogEntry le;
	char *hostnames[]={
		"www.hostname1.nl",
		"www.hostname2.ca",
		"www.hostname3.ie",
		"www.hostname4.test",
		"www.hostname5.info",
		"www.hostname6.what"		
	};
	
	srand(GetTickCount());
	int ratio=600;
	for (int c=0;c<10000000;c++)
	{
		myTime+=0.0001f*(float)(rand()%1000);
		int hn=rand()%(sizeof(hostnames)/sizeof(char*));
		strcpy_s(le.hostname,100,hostnames[hn]);
		le.cacheLookupStatus=(CacheLookupStatus) (rand()%1000)<=ratio ? Hit : Miss; 
		le.endTime=(time_t) myTime;
		strcpy_s(le.contenttype,80,(rand()%3==0) ? "text/html" : "image/jpeg");
		le.httpCode=200;
		le.payloadsize=14000+(rand()%14000);
		strcpy_s(le.url,400,"/ditblijftconstart");
		
		sprintf_s(le.useraddress,100,"%d.%d.%d.%d",rand()%3,rand()%3,rand()%3,rand()%10);

				
		hm->Aggregate(&le);
		bb->Aggregate(&le);
		if (myTime>=nextCheck)
		{
			nextCheck+=800;
			//hm->Dump();
		}
	}


	printf("%f\n",myTime);

	

	


    return 0;

}

