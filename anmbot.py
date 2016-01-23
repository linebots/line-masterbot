# anm-bot.py by @nieltg

from common import default
from datetime import datetime

import time, sys, io, socket
import re
import sqlite3

print ("anmbot.py by @nieltg")
print ("")

#
# Constants declarations.

ANM_LOG_FILE      = "anm.log"
ANM_DATABASE_FILE = "anm.db"
ANM_MAX_NET_ERROR = 3
ANM_RECOVER_DELAY = 300

ANM_GROUP_NAME    = "[STEI] Angel&Mortal"
ANM_MODERATOR_IDS = []

ANM_RE_KEYWORD    = '\s*(?i)anm\.(?P<key>\w+)\s*'

ANM_MSG_INVALID     = "[BOT] Invalid command: %(cmd)s"
ANM_MSG_BAD_USAGE   = "[BOT] Invalid usage of command."
ANM_MSG_NOTICE_PONG = "[BOT] Your account-info has been logged. Thanks!"

#
# Method definitions.

factory = default.init_factory ()

re_keyword = re.compile (ANM_RE_KEYWORD)

client = None

profile = None
group = None

log_handle = None
log_mode = None

db_handle = None

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
	rk = re_keyword.match (msg)
	if rk <> None:
		key = rk.group ('key').lower ()
		msg = msg[rk.end ():]
	return (key, msg)

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
			key, msg = process_keyword (message.text)
			
			if key <> None:
				optable = optable_user
				
				if sender.id in ANM_MODERATOR_IDS:
					optable = optable_priv
				
				try:
					if key in optable:
						optable[key] (sender, receiver, msg)
					else:
						sender.sendMessage (ANM_MSG_INVALID % {'cmd': key})
					
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

def db_open ():
	global db_handle
	
	db_handle = sqlite3.connect (ANM_DATABASE_FILE)
	db_handle.executescript ('''
	CREATE TABLE "users" IF NOT EXISTS (
	  acc_id TEXT UNIQUE NOT NULL,
	  name TEXT NOT NULL,
	  mortal_name TEXT NOT NULL,
	  enable INTEGER DEFAULT 0);
	CREATE TABLE "msg_log" IF NOT EXISTS (
	  time DATETIME DEFAULT CURRENT_TIMESTAMP,
	  acc_id TEXT NOT NULL,
	  mortal_name TEXT NOT NULL,
	  message TEXT NOT NULL,
	  FOREIGN KEY (acc_id) REFERENCES users (acc_id) ON UPDATE RESTRICT ON DELETE RESTRICT);
	PRAGMA foreign_keys = ON;
	''')

def db_close ():
	global db_handle
	
	db_handle.close ()
	db_handle = None

def db_user_register (acc_id, name, mortal_name):
	global db_handle
	
	query = "INSERT OR REPLACE INTO users (acc_id, name, mortal_name) VALUES (?,?,?)"
	param = (acc_id, name, mortal_name)
	
	cursor = db_handle.cursor ()
	cursor.execute (query, param)
	db_handle.commit ()
	
	return cursor.lastrowid

def db_user_enable (rowid, value=True):
	global db_handle
	
	query = "UPDATE users SET enable=? WHERE _ROWID_=?"
	param = (int (value), rowid)
	
	db_handle.execute (query, param)
	db_handle.commit ()

def db_query_mortal (acc_id):
	global db_handle
	
	query = "SELECT mortal_name FROM users WHERE acc_id=?, enable"
	param = (acc_id)
	
	cursor = db_handle.cursor ()
	cursor.execute (query, param)
	data = cursor.fetchone ()
	
	if data == None: return None
	return data["mortal_name"]

def db_log_message (acc_id, mortal_name, msg):
	global db_handle
	
	query = "INSERT INTO msg_log (acc_id, mortal_name, message) VALUES (?,?,?)"
	param = (acc_id, mortal_name, msg)
	
	c = db_handle.cursor ()
	c.execute (query, param)
	db_handle.commit ()

#
# Operation defintions.

def op_noticeme (sender, receiver, msg):
	log_write ("noticeme: %s (%s)" % (sender.name, sender.id), mode="noticeme")
	sender.sendMessage (ANM_MSG_NOTICE_PONG)

optable_default = {
	"noticeme" : op_noticeme
}

optable_user = optable_default.copy ()
optable_user.update ({
})

optable_priv = optable_default.copy ()
optable_priv.update ({
})

#
# Main application.

log_open ()
log_write ("AnM-Bot started at %s" % (str (datetime.now ())), mode="init")

connect ()
log_write ("init: Connection established.", mode="init")

db_open ()

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

db_close ()

log_write ("AnM-Bot stopped at %s" % str (datetime.now ()), mode="init")
log_close ()

