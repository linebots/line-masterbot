# default.py by @nieltg

from .factory import ClientFactory
from .cache import TokenCache


def _init_factory ():
	
	factory = ClientFactory ()
	cache = TokenCache ()
	
	if cache.is_available ():
		cache.inject (factory)
	else:
		factory.auth ()
		cache.store (factory)
	
	return factory


def init_client ():
	
	factory = _init_factory ()
	
	cli_comm = factory.get_command_client ()
	
	print ("done.")
	print ("")
	
	return cli_comm


def init_client_poll ():
	
	factory = _init_factory ()
	
	cli_comm = factory.get_command_client ()
	cli_poll = factory.get_poll_client ()
	
	print ("done.")
	print ("")
	
	return ( cli_comm, cli_poll )

