# client.py by @nieltg

from line import LineClient

LINE_COMM_PATH  = "/S4"
LINE_POLL_PATH = "/P4"


class LineCommandClient (LineClient):
	
	def __init__ (self, *args, **kwargs):
		
		LineClient.__init__ (self, *args, **kwargs)
		self.transport.path = LINE_COMM_PATH


class LinePollClient (LineClient):
	
	def __init__ (self, *args, **kwargs):
		
		self._headers['Connection'] = "Keep-Alive"
		
		LineClient.__init__ (self, *args, **kwargs)
		self.transport.path = LINE_POLL_PATH

