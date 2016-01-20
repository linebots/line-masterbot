# default.py by @nieltg

from .factory import ClientFactory
from .cache import TokenCache


def init_factory ():
	
	factory = ClientFactory ()
	cache = TokenCache ()
	
	if cache.is_available ():
		cache.inject (factory)
	else:
		factory.auth ()
		cache.store (factory)
	
	return factory


def init_client ():
	
	factory = init_factory ()
	
	cli_comm = factory.get_client ()
	
	print ("done.")
	print ("")
	
	return cli_comm

