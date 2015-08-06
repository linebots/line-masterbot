# cache.py by @nieltg

from Crypto.Cipher import AES
import getpass, hashlib, io, os


class TokenCache (object):
	
	def __init__ (self, tokfile="authtoken.dat"):
		self.token_file = tokfile
	
	def is_available (self):
		return os.path.isfile (self.token_file)
	
	def inject (self, factory):
		
		handle = io.open (self.token_file, "rb")
		
		tokn_h = handle.read (16)
		tokn_e = handle.read ()
		handle.close ()
		
		print ("cache: %s is read." % self.token_file)
		
		key = getpass.getpass ("Encrypt Key [16]: ").rjust (16)
		print ("")
		
		cipher = AES.new (key, AES.MODE_ECB)
		tokn_r = cipher.decrypt (tokn_e).lstrip ()
		
		if hashlib.md5 (tokn_r).digest () != tokn_h:
			raise Exception("decryption failed")
		
		factory.authToken = tokn_r
	
	def store (self, factory):
		
		tokn_r = factory.authToken
		
		key = getpass.getpass ("Encrypt Key [16]: ").rjust (16)
		
		e_m = None
		if len (key) > 16:
			e_m = "cache: key exceed 16-chars length, not caching..."
		if key.strip () == "":
			e_m = "cache: no encryption-key provided, not caching..."
		if e_m != None:
			print (e_m)
			print ("")
			os.unlink (self.token_file)
			return
		
		t_div, t_mod = divmod (len (tokn_r), 16)
		t_pad = 16 if t_mod > 0 else 0
		
		tokn_d = tokn_r.rjust ((t_div * 16) + t_pad)
		
		cipher = AES.new (key, AES.MODE_ECB)
		tokn_e = cipher.encrypt (tokn_d)
		
		handle = io.open (self.token_file, "wb")
		
		handle.write (hashlib.md5 (tokn_r).digest ())
		handle.write (tokn_e)
		handle.close ()
		
		print ("written %s." % self.token_file)
		print ("")

