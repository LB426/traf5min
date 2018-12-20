#!/bin/sh

/usr/local/sbin/ipacctctl rl0_ip_acct:rl0 checkpoint
/usr/local/sbin/ipacctctl rl0_ip_acct:rl0 show
/usr/local/sbin/ipacctctl rl0_ip_acct:rl0 clear
