import os
import sys
import shlex
import tempfile
import subprocess
from config import *
from common import file_size

class Extractor:
	'''
	Extractor class, responsible for extracting files from the target file and executing external applications, if requested.
	An instance of this class is accessible via the Binwalk.extractor object.

	Example usage:

		import binwalk
		
		bw = binwalk.Binwalk()

		# Create extraction rules for scan results containing the string 'gzip compressed data' and 'filesystem'.
		# The former will be saved to disk with a file extension of 'gz' and the command 'gunzip <file name on disk>' will be executed (note the %e placeholder).
		# The latter will be saved to disk with a file extension of 'fs' and no command will be executed.
		# These rules will take precedence over subsequent rules with the same match string.
		bw.extractor.add_rule(['gzip compressed data:gz:gunzip %e', 'filesystem:fs'])

		# Load the extraction rules from the default extract.conf file(s).
		bw.extractor.load_defaults()

		# Run the binwalk scan.
		bw.scan('firmware.bin')
		
	'''
	# Extract rules are delimited with a colon.
	# <case insensitive matching string>:<file extension>[:<command to run>]
	RULE_DELIM = ':'

	# Comments in the extract.conf files start with a pound
	COMMENT_DELIM ='#'

	# Place holder for the extracted file name in the command 
	FILE_NAME_PLACEHOLDER = '%e'

	def __init__(self, verbose=False):
		'''
		Class constructor.
	
		@verbose - Set to True to display the output from any executed external applications.

		Returns None.
		'''
		self.config = Config()
		self.enabled = False
		self.delayed = False
		self.verbose = verbose
		self.extract_rules = {}
		self.remove_after_execute = False

	def add_rule(self, rule):
		'''
		Adds a set of rules to the extraction rule list.

		@rule - Rule string, or list of rule strings, in the format <case insensitive matching string>:<file extension>[:<command to run>]

		Returns None.
		'''
		r = {
			'extension'	: '',
			'cmd'		: ''
		}

		if type(rule) != type([]):
			rules = [rule]
		else:
			rules = rule

		for rule in rules:
			r['cmd'] = ''
			r['extension'] = ''

			try:
				values = self._parse_rule(rule)
				match = values[0].lower()
				r['extension'] = values[1]
				r['cmd'] = values[2]
			except:
				pass

			# Verify that the match string and file extension were retrieved.
			# Only add the rule if it is a new one (first come, first served).
			if match and r['extension'] and not self.extract_rules.has_key(match):
				self.extract_rules[match] = {}
				self.extract_rules[match]['cmd'] = r['cmd']
				self.extract_rules[match]['extension'] = r['extension']
				# Once any rule is added, set self.enabled to True
				self.enabled = True

	def enable_delayed_extract(self, tf=None):
		'''
		Enables / disables the delayed extraction feature.
		This feature ensures that certian supported file types will not contain extra data at the end of the
		file when they are extracted, but also means that these files will not be extracted until the end of the scan.

		@tf - Set to True to enable, False to disable. 

		Returns the current delayed extraction setting.
		'''
		if tf is not None:
			self.delayed = tf
		return self.delayed

	def load_from_file(self, fname):
		'''
		Loads extraction rules from the specified file.

		@fname - Path to the extraction rule file.
		
		Returns None.
		'''
		try:
			# Process each line from the extract file, ignoring comments
			for rule in open(fname).readlines():
				self.add_rule(rule.split(self.COMMENT_DELIM, 1)[0])
		except Exception, e:
			raise Exception("Extractor.load_from_file failed to load file '%s': %s" % (fname, str(e)))

	def load_defaults(self):
		'''
		Loads default extraction rules from the user and system extract.conf files.

		Returns None.
		'''
		# Load the user extract file first to ensure its rules take precedence.
		extract_files = [
			self.config.paths['user'][self.config.EXTRACT_FILE],
			self.config.paths['system'][self.config.EXTRACT_FILE],
		]

		for extract_file in extract_files:
			try:
				self.load_from_file(extract_file)
			except Exception, e:
				if self.verbose:
					raise Exception("Extractor.load_defaults failed to load file '%s': %s" % (extract_file, str(e)))

	def cleanup_extracted_files(self, tf=None):
		'''
		Set the action to take after a file is extracted.

		@tf - If set to True, extracted files will be cleaned up after running a command against them.
		      If set to False, extracted files will not be cleaned up after running a command against them.
		      If set to None or not specified, the current setting will not be changed.

		Returns the current cleanup status (True/False).
		'''
		if tf is not None:
			self.remove_after_execute = tf

		return self.remove_after_execute
	
	def extract(self, offset, description, file_name, size, name=None):
		'''
		Extract an embedded file from the target file, if it matches an extract rule.
		Called automatically by Binwalk.scan().

		@offset      - Offset inside the target file to begin the extraction.
		@description - Description of the embedded file to extract, as returned by libmagic.
		@file_name   - Path to the target file.
		@size        - Number of bytes to extract.
		@name        - Name to save the file as.

		Returns the name of the extracted file (blank string if nothing was extracted).
		'''
		cleanup_extracted_fname = True

		rule = self._match(description)
		if rule is not None:
			fname = self._dd(file_name, offset, size, rule['extension'], output_file_name=name)
			if rule['cmd']:

				# Many extraction utilities will extract the file to a new file, just without
				# the file extension (i.e., myfile.7z => myfile). If the presumed resulting
				# file name already exists before executing the extract command, do not attempt 
				# to clean it up even if its resulting file size is 0.
				if self.remove_after_execute:
					extracted_fname = os.path.splitext(fname)[0]
					if os.path.exists(extracted_fname):
						cleanup_extracted_fname = False

				# Execute the specified command against the extracted file
				self._execute(rule['cmd'], fname)

				# Only clean up files if remove_after_execute was specified				
				if self.remove_after_execute:

					# Remove the original file that we extracted
					try:
						os.unlink(fname)
					except:
						pass

					# If the command worked, assume it removed the file extension from the extracted file

					# If the extracted file name file exists and is empty, remove it
					if cleanup_extracted_fname and os.path.exists(extracted_fname) and file_size(extracted_fname) == 0:
						try:
							os.unlink(extracted_fname)
						except:
							pass
		else:
			fname = ''

		return fname

	def delayed_extract(self, results, file_name, size):
		'''
		Performs a delayed extraction (see self.enable_delayed_extract).
		Called internally by Binwalk.Scan().

		@results   - A list of dictionaries of all the scan results.
		@file_name - The path to the scanned file.
		@size      - The size of the scanned file.

		Returns an updated results list containing the names of the newly extracted files.
		'''
		index = 0
		info_count = 0
		nresults = results

		for (offset, infos) in results:
			info_count = 0

			for info in infos:
				ninfos = infos

				if info['delay']:
					end_offset = self._entry_offset(index, results, info['delay'])
					if end_offset == -1:
						extract_size = size
					else:
						extract_size = (end_offset - offset)

					ninfos[info_count]['extract'] = self.extract(offset, info['description'], file_name, extract_size, info['name'])
					nresults[index] = (offset, ninfos)

				info_count += 1

			index += 1
		
		return nresults

	def _entry_offset(self, index, entries, description):
		'''
		Gets the offset of the first entry that matches the description.

		@index       - Index into the entries list to begin searching.
		@entries     - Dictionary of result entries.
		@description - Case insensitive description.

		Returns the offset, if a matching description is found.
		Returns -1 if a matching description is not found.
		'''
		description = description.lower()

		for (offset, infos) in entries[index:]:
			for info in infos:
				if info['description'].lower().startswith(description):
					return offset
		return -1

	def _match(self, description):
		'''
		Check to see if the provided description string matches an extract rule.
		Called internally by self.extract().

		@description - Description string to check.

		Returns the associated rule dictionary if a match is found.
		Returns None if no match is found.
		'''
		description = description.lower()

		for (m, rule) in self.extract_rules.iteritems():
			if m in description:
				return rule
		return None

	def _parse_rule(self, rule):
		'''
		Parses an extraction rule.

		@rule - Rule string.

		Returns an array of ['<case insensitive matching string>', '<file extension>', '<command to run>'].
		'''
		return rule.strip().split(self.RULE_DELIM, 2)

	def _dd(self, file_name, offset, size, extension, output_file_name=None):
		'''
		Extracts a file embedded inside the target file.

		@file_name        - Path to the target file.
		@offset           - Offset inside the target file where the embedded file begins.
		@size             - Number of bytes to extract.
		@extension        - The file exension to assign to the extracted file on disk.
		@output_file_name - The requested name of the output file.

		Returns the extracted file name.
		'''
		# Default extracted file name is <hex offset>.<extension>
		altname = "%X.%s" % (offset, extension)
		
		if not output_file_name or output_file_name is None:
			fname = altname
		else:
			fname = "%s.%s" % (output_file_name, extension)
	
		# Sanitize output file name of invalid/dangerous characters (like file paths)	
		fname = os.path.basename(fname)

		try:
			# Open the target file and seek to the offset
			fdin = open(file_name, "rb")
			fdin.seek(offset)
			
			# Open the extracted file
			try:
				fdout = open(fname, "wb")
			except:
				# Fall back to the alternate name if the requested name fails
				fname = altname
				fdout = open(fname, "wb")

			# Read size bytes from target file and write it to the extracted file
			fdout.write(fdin.read(size))

			# Cleanup
			fdout.close()
			fdin.close()
		except Exception, e:
			raise Exception("Extractor.dd failed to extract data from '%s' to '%s': %s" % (file_name, fname, str(e)))
		
		return fname

	def _execute(self, cmd, fname):
		'''
		Execute a command against the specified file.

		@cmd   - Command to execute.
		@fname - File to run command against.

		Returns None.
		'''
		tmp = None

		# If not in verbose mode, create a temporary file to redirect stdout and stderr to
		if not self.verbose:
			tmp = tempfile.TemporaryFile()

		try:
			# Replace all instances of FILE_NAME_PLACEHOLDER in the command with fname
			cmd = cmd.replace(self.FILE_NAME_PLACEHOLDER, fname)

			# Execute.
			subprocess.call(shlex.split(cmd), stdout=tmp, stderr=tmp)
		except Exception, e:
			sys.stderr.write("WARNING: Extractor.execute failed to run '%s': %s\n" % (cmd, str(e)))
		
		if tmp is not None:
			tmp.close()

	

