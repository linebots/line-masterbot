# factory.py by @nieltg

from .client import LineCommandClient, LinePollClient
import getpass


class ClientFactory (object):
	
	def __init__ (self, is_mac=True, com_name="python"):
		
		self.authToken = None
		
		#self._cli_comm = None
		#self._cli_pool = None
		
		self.kwargs = {
			"is_mac"   : is_mac,
			"com_name" : com_name
		}
	
	def _login (self, user, pasw):
		raise NotImplementedError ()
	
	def auth (self):
		
		user = raw_input ("Username: ")
		pasw = getpass.getpass ("Password: ")
		
		print ("")
		print ("Logging in... ")
		
		self._login (user, pasw)
	
	def get_command_client (self):
		print ("Preparing client object... ")
		return LineCommandClient (authToken=self.authToken, **self.kwargs)
	
	def get_poll_client (self):
		print ("Preparing long-poll object... ")
		return LinePollClient (authToken=self.authToken, **self.kwargs)

