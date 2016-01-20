# client.py by @nieltg

from line import LineClient
from curve import CurveThrift
from curve.ttypes import OperationType, TalkException
from thrift.protocol import TCompactProtocol

from .transport import THttpPersist

LINE_CMD_PATH = "/S4"
LINE_POLL_PATH = "/P4"


class LineClient2 (LineClient):
	
	def __init__ (self, *args, **kwargs):
		self._headers['Connection'] = "Keep-Alive"
		
		LineClient.__init__ (self, *args, **kwargs)
		self.transport.path = LINE_CMD_PATH
	
	def login (self):
		raise NotImplementedError ()
	
	def tokenLogin (self):
		self.transport = THttpPersist (self.LINE_HTTP_URL)
		self.transport.setCustomHeaders (self._headers)

		self.protocol = TCompactProtocol.TCompactProtocol (self.transport)
		self._client  = CurveThrift.Client (self.protocol)
	
	def _fetchOperations (self, revision, count=50):
		try:
			self.transport.path = LINE_POLL_PATH
			return LineClient._fetchOperations (self, revision, count)
		finally:
			self.transport.path = LINE_CMD_PATH
	
	#def longPoll2 (self, count=50):
	#	try:
	#		operations = self._fetchOperations(self.revision, count)
	#	except EOFError:
	#		return
	#	except TalkException as e:
	#		if e.code == 9:
	#			self.raise_error("user logged in to another machine")
	#		else:
	#			return
	#	except Exception:
	#		return
	#	
	#	for operation in operations:
	#		self.revision = max(operation.revision, self.revision)
	#	
	#	return operations

