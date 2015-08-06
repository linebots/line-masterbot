# echobot.py by @nieltg
# Thanks to STEI LINE EXPERIMENT

from common import init_client_poll
from line import LineClient
import getpass, time, sys, io


print ("echobot.py by @nieltg")
print ("Thanks to STEI LINE EXPERIMENT")
print ("")

#
# Step 1: Get credentials

client, poll = init_client_poll ()

#
# Step 2: 

CFG_GROUP_NAME = 'STEI LINE EXPERIMENT'
CFG_ECHO_TIMES = 5
CFG_MAX_NETW_E = 3

group = client.getGroupByName (CFG_GROUP_NAME)
group.sendMessage ("[BOT] This is EchoBot v2\nI will echo %s msgs and quit." % str(CFG_ECHO_TIMES))

print ("EchoBot is started...")
print ("")

#
# Step 3:

n_netw = CFG_MAX_NETW_E
n_loop = CFG_ECHO_TIMES
n_cont = True

def network_except (e):
	
	global n_netw
	n_netw = n_netw - 1
	
	if n_netw < 0:
		group.sendMessage ("[BOT] network error (limit reached), bye!")
		print ("echobot: network error (limit reached)")
		
		raise e
	
	print ("echobot: network error (%i more attempts), retrying..." % n_netw)
	time.sleep (1)


while n_cont:
	
	op_list = []
	
	try:
		for op in poll.longPoll():
			op_list.append(op)
		n_netw = CFG_MAX_NETW_E
	except socket.gaierror, e:
		network_except (e)
	
	print ("echobot: %s operation retrieved." % str (len (op_list)))
	
	for op in op_list:
		sender   = op[0]
		receiver = op[1]
		message  = op[2]
		
		if receiver.id == group.id:
			
			n_loop = n_loop - 1
			
			print ("echobot: %s: %s" % (sender.name, message.text))
			
			try:
				group.sendMessage ("[BOT: %i] %s" % (n_loop, message.text))
				n_netw = CFG_MAX_NETW_E
			except socket.gaierror, e:
				network_except (e)
			
			if n_loop < 1:
				n_cont = False
				break


group.sendMessage ("[BOT] test is finished, bye!")
group.sendMessage ("[BOT] hav a nice day :)")

print ("echobot: echo limit reached!")
print ("")

poll.transport.close ()
client.transport.close ()

