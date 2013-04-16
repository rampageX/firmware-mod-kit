import urllib2
from config import *

class Update:
	'''
	Class for updating Binwalk configuration and signatures files from the subversion trunk.

	Example usage:

		from binwalk import Update

		Update().update()
	'''
	BASE_URL = "http://binwalk.googlecode.com/svn/trunk/src/binwalk/"
	MAGIC_PREFIX = "magic/"
	CONFIG_PREFIX = "config/"

	def __init__(self):
		'''
		Class constructor.
		'''
		self.config = Config()

	def update(self):
		'''
		Updates all system wide signatures and config files.

		Returns None.
		'''
		self.update_binwalk()
		self.update_bincast()
		self.update_binarch()
		self.update_extract()

	def _do_update_from_svn(self, prefix, fname):
		'''
		Updates the specified file to the latest version of that file in SVN.

		@prefix - The URL subdirectory where the file is located.
		@fname  - The name of the file to update.

		Returns None.
		'''
		url = self.BASE_URL + prefix + fname
		
		try:
			data = urllib2.urlopen(url).read()
			open(self.config.paths['system'][fname], "wb").write(data)
		except Exception, e:
			raise Exception("Update._do_update_from_svn failed to update file '%s': %s" % (url, str(e)))

	def update_binwalk(self):
		'''
		Updates the binwalk signature file.

		Returns None.
		'''
		self._do_update_from_svn(self.MAGIC_PREFIX, self.config.BINWALK_MAGIC_FILE)
	
	def update_bincast(self):
		'''
		Updates the bincast signature file.

		Returns None.
		'''
		self._do_update_from_svn(self.MAGIC_PREFIX, self.config.BINCAST_MAGIC_FILE)
	
	def update_binarch(self):
		'''
		Updates the binarch signature file.
	
		Returns None.
		'''
		self._do_update_from_svn(self.MAGIC_PREFIX, self.config.BINARCH_MAGIC_FILE)

	def update_extract(self):
		'''
		Updates the extract.conf file.
	
		Returns None.
		'''
		self._do_update_from_svn(self.CONFIG_PREFIX, self.config.EXTRACT_FILE)

