# anm-bot.py by @nieltg

from common import default
from datetime import datetime

import time, sys, io, socket
import re

print ("anmbot.py by @nieltg")
print ("")

#
# Constants declarations.

ANM_LOG_FILE      = "anm.log"
ANM_MAX_NET_ERROR = 3
ANM_RECOVER_DELAY = 300

ANM_GROUP_NAME    = "[STEI] Angel&Mortal"
ANM_KEYWORD_REGEX = '\s*(?i)anm\.(?P<key>\w+)\s*'

ANM_MSG_HEADER    = "[BOT AnM-fwd]\n"
ANM_MSG_PROCESS   = "[BOT] Message has been processed."
ANM_MSG_UNABLE    = "[BOT] Unable to process message. Sorry."

ANM_ACC_BLACKLIST = []

#
# Method definitions.

factory = default.init_factory ()
keyword_re = re.compile (ANM_KEYWORD_REGEX)

client = None

profile = None
group = None

log_handle = None
log_mode = None

def connect (last_rev=None):
	global factory, client, profile, group, net_error
	
	client = factory.get_client ()
	
	print ("done.")
	print ("")
	
	profile = client.getProfile ()
	group = client.getGroupByName (ANM_GROUP_NAME)
	
	net_error = ANM_MAX_NET_ERROR
	
	if last_rev <> None:
		client.revision = last_rev

def reconnect (exit=False):
	global client, profile, group
	
	profile = None
	group = None
	
	last_rev = client.revision
	client.transport.close ()
	
	client = None
	
	if not exit:
		connect (last_rev)

def network_except (ne):
	global net_error
	
	if net_error <= 0:
		raise ne
	
	net_error = net_error - 1

def process_keyword (msg):
	key = None
	if msg == None: return None
	rk = keyword_re.match (msg)
	if rk <> None:
		key = rk.group ('key').lower ()
		msg = msg[rk.end ():]
	return (key, msg)

def process_msg (msg):
	key, n_msg = process_keyword (msg)
	
	if key <> "fwd": return None
	return ANM_MSG_HEADER + n_msg

def listen_evt ():
	global client, net_error
	
	try:
		op_list = []
		
		for op in client.longPoll ():
			op_list.append (op)
		
		net_error = ANM_MAX_NET_ERROR
		return op_list
		
	except socket.gaierror as ne:
		network_except (ne)

def handle_evt (op_list):
	global profile, group
	
	for op in op_list:
		sender   = op[0]
		receiver = op[1]
		message  = op[2]
		
		# Bugfix: disable 'Letter Sealing' so message can be read!
		
		if receiver.id == profile.id:
			msg = process_msg (message.text)
			
			if msg <> None:
				try:
					if sender.id in ANM_ACC_BLACKLIST:
						log_entry (sender.id, sender.name, "[BLACKLIST]")
						
						sender.sendMessage (ANM_MSG_UNABLE)
						return
					
					log_entry (sender.id, sender.name, msg)
					
					group.sendMessage (msg)
					sender.sendMessage (ANM_MSG_PROCESS)
					
				except socket.gaierror as ne:
					network_except (ne)

def log_open ():
	global log_handle, log_mode
	
	log_handle = io.open (ANM_LOG_FILE, "a")
	log_mode = None

def log_close ():
	global log_handle
	
	log_handle.close ()
	log_handle = None

def log_write (msg, mode="custom"):
	global log_handle, log_mode
	
	if (log_mode <> None) and (log_mode <> mode):
		log_handle.write (u"\n")
	log_mode = mode
	
	n_msg = u"%s\n" % (msg)
	
	print (n_msg)
	log_handle.write (n_msg)
	log_handle.flush ()

def log_entry (acc_id, name, msg):
	n_msg = msg.replace ('\n', ' ')
	log_write ("%s (%s): %s" % (name, acc_id, n_msg), mode="entry")

#
# Main application.

log_open ()
log_write ("AnM-Bot started at %s" % (str (datetime.now ())), mode="init")

connect ()
log_write ("init: Connection established.", mode="init")

while True:
	
	try:
		op_list = listen_evt ()
		handle_evt (op_list)
		
	except KeyboardInterrupt as ke:
		log_write ("init: KeyboardInterrupt is raised!", mode="init")
		
		reconnect (exit=True)
		break
		
	except:
		log_write ("except: %s" % (str (sys.exc_info()[0])), mode="except")
		log_write ("except: Sleeping in %d and recover." % (ANM_RECOVER_DELAY))
		
		time.sleep (ANM_RECOVER_DELAY)
		reconnect ()

log_write ("AnM-Bot stopped at %s" % str (datetime.now ()), mode="init")
log_close ()

