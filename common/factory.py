# factory.py by @nieltg

from .client import LineClient2
import getpass


class ClientFactory (object):
	
	def __init__ (self, is_mac=True, com_name="python"):
		
		self.authToken = None
		
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
	
	def get_client (self):
		print ("Preparing client object... ")
		# TODO: do client object caching here.
		return LineClient2 (authToken=self.authToken, **self.kwargs)

