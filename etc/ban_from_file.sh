#!/bin/bash                                                                                                                                                                                                                                  

while true; do for IP in `tail -n 10000 /usr/local/trafficserver/logs/ban_ip_list.log | cut -f1 -d" " | uniq -c | sort -n | tail -n 500 | awk '{print $2}'`; do
      ## Check if this IP is already banned                                                                                                                                                                                                  
      #echo $IP                                                                                                                                                                                                                              
      if ! iptables -L INPUT -v -n | grep $IP >/dev/null
      then
      ## If it is not banned, ban it                                                                                                                                                                                                         
          echo $IP
         iptables -I INPUT -j DROP -s $IP
      fi
    done
    sleep 5
 done
