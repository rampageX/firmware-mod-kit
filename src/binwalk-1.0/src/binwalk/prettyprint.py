import sys
import hashlib
from datetime import datetime

class PrettyPrint:
        '''
        Class for printing Binwalk results to screen/log files.
	
	An instance of PrettyPrint is available via the Binwalk.display object.
	The PrettyPrint.results() method is of particular interest, as it is suitable for use as a Binwalk.scan() callback function,
	and can be used to print Binwalk.scan() results to stdout, a log file, or both.

	Example usage:

		import binwalk

		bw = binwalk.Binwalk()

		bw.display.header()
		bw.scan('firmware.bin', callback=bw.display.results)
		bw.display.footer()
        '''

        def __init__(self, log=None, quiet=False, bwalk=None, verbose=0):
                '''
                Class constructor.
		
		@log     - Output log file.
		@quiet   - If True, results will not be displayed to screen.
		@bwalk   - The Binwalk class instance.
		@verbose - If set to True, target file information will be displayed when file_info() is called.

		Returns None.
		'''
                self.fp = None
                self.log =log
                self.quiet = quiet
                self.binwalk = bwalk
                self.verbose = verbose

                if self.log is not None:
                        self.fp = open(log, "w")

        def __del__(self):
		'''
		Class deconstructor.
		'''
		# Close the log file.
                try:
                        self.fp.close()
                except:
                        pass

        def _log(self, data):
		'''
		Log data to the log file.
		'''
                if self.fp is not None:
                        self.fp.write(data)

        def _pprint(self, data):
		'''
		Print data to stdout and the log file.
		'''
                if not self.quiet:
                        sys.stdout.write(data)
                self._log(data)

        def _file_md5(self, file_name):
		'''
		Generate an MD5 hash of the specified file.
		'''
                md5 = hashlib.md5()
                
                with open(file_name, 'rb') as f:
                        for chunk in iter(lambda: f.read(128*md5.block_size), b''):
                                md5.update(chunk)

                return md5.hexdigest()
                        
        def file_info(self, file_name):
		'''
		Prints detailed info about the specified file, including file name, scan time and the file's MD5 sum.
		Called internally by self.header if self.verbose is not 0.

		@file_name - The path to the target file.

		Returns None.
		'''
                self._pprint("\n")
                self._pprint("Scan Time:     %s\n" % datetime.now().strftime("%Y-%m-%d %H:%M:%S"))
		self._pprint("Signatures:    %d\n" % self.binwalk.parser.signature_count)
                self._pprint("Target File:   %s\n" % file_name)
                self._pprint("MD5 Checksum:  %s\n" % self._file_md5(file_name))

        def header(self, file_name=None):
		'''
		Prints the Binwalk header, typically used just before starting a scan.

		@file_name - If specified, and if self.verbose > 0, then detailed file info will be included in the header.

		Returns None.
		'''
                if self.verbose and file_name is not None:
                        self.file_info(file_name)

                self._pprint("\nDECIMAL   \tHEX       \tDESCRIPTION\n")
                self._pprint("-------------------------------------------------------------------------------------------------------\n")

        def footer(self):
		'''
		Prints the Binwalk footer, typically used just after completing a scan.

		Returns None.
		'''
                self._pprint("\n")

        def results(self, offset, results):
		'''
		Prints the results of a scan. Suitable for use as a callback function for Binwalk.scan().

		@offset  - The offset at which the results were found.
		@results - A list of libmagic result strings.
		
		Returns None.
		'''
                offset_printed = False

                for info in results:
			# Check for any grep filters before printing
                        if self.binwalk is not None and self.binwalk.filter.grep(info['description']):
				# Only display the offset once per list of results
                                if not offset_printed:
                                        self._pprint("%-10d\t0x%-8X\t%s\n" % (offset, offset, info['description']))
                                        offset_printed = True
                                else:
                                        self._pprint("%s\t  %s\t%s\n" % (' '*10, ' '*8, info['description']))

