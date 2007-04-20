#!/usr/bin/env python

# Written by Diego Ongaro, 2007
# This code is in the public domain

import os, time
import codecs

class i18nFile:
	def __init__(self, data=None):
		self.header = [] # list of strings, beginning with '#'
		if data is None:
			self.map = []
		else:
			self.map = list(data)

	def __contains__(self, needle):
		for id, str in self.map:
			if id == needle:
				return True
		return False

	def __getitem__(self, needle):
		for id, str in self.map:
			if id == needle:
				return str
		raise KeyError

	def __iter__(self):
		return self.map.__iter__()

	def __len__(self):
		return len(self.map)

	def __str__(self):
		ret = []
		for (id, str) in self.map:
			ret.append("%s => %s" % (id, str))
		return '\n'.join(ret)

	def _allow(self, id, str):
		return True

	def append(self, id, str):
		if self._allow(id, str):
			self.map.append((id, str))

	def intersect(self, pot):
		ret = i18nFile(filter(lambda (id, str): id in pot, self.map))
		ret.header = self.header
		return ret

	def replace_id(self, old, new):
		old = 'msgid "%s"' % old
		new = 'msgid "%s"' % new
		for k, (id, str) in enumerate(self.map):
			if id == old:
				self.map[k] = (new, str)

	def to_file(self, filename):
		fp = codecs.open(filename, 'w', "utf-8")

		for line in self.header:
			fp.write('%s\n' % line)

		fp.write('msgid ""\n')
		fp.write('msgstr ""\n\n')
		fp.write('"Content-Type: text/plain; charset=UTF-8\\n"\n')
		fp.write('"Content-Transfer-Encoding: 8bit\\n"\n\n')

		for id, str in self.map:   #  [('msgid ""', 'msgstr ""')] + self.map:
			fp.write("%s\n%s\n\n" % (id, str))
		
		fp.close()

class PoFile(i18nFile):
	def _allow(self, id, str):
		return id != 'msgid ""'

class CleanPoFile(i18nFile):
	def _allow(self, id, str):
		return id != 'msgid ""' and str != 'msgstr ""'

def parse_po(filename, PoClass):
	po_file = PoClass()
	
	lines = codecs.open(filename, "r", "utf-8").readlines()
	lines = map(unicode.strip, lines)

	for line in lines:
		if line[0] == '#':
			po_file.header.append(line)
		if line.startswith('msgid'):
			break

	# doesn't do msgid_plural
	lines = filter(lambda line: line and (line.startswith('msgid ') or line.startswith('msgstr ')), lines)
	
	prev_id = None
	for line in lines:
		if line.startswith('msgid '):
			prev_id = line
		elif line.startswith('msgstr '):
			assert prev_id is not None
			po_file.append(prev_id, line)
			prev_id = None

	return po_file

def priority_make_po(pot, po_list):
	po_file = PoFile()
	po_file.header = ['# This file was automatically generated for the xfce4-places-plugin (%s)' % time.localtime()[0], \
			  '# This file is distributed under the same license as the xfce4-places-plugin package', \
			  '# It is based on the following:']
	for po in po_list:
		po_file.header.append('####')
		po_file.header += po.header
	po_file.header.append('####')
	po_file.header.append('')

	misses = 0
	for id, str in pot:
		for po in po_list:
			if id in po:
				po_file.append(id, po[id])
				break
		else:
			po_file.append(id, str)
			misses += 1
	return po_file, (misses, len(pot))


pot = parse_po('../po/xfce4-places-plugin.pot', PoFile)

gnome_po_files =  filter(lambda filename: filename.endswith('.po'), os.listdir('gnome-panel'))
thunar_po_files = filter(lambda filename: filename.endswith('.po'), os.listdir('thunar'))

linguas = []

for po_file in set(gnome_po_files + thunar_po_files):
	
	if po_file in thunar_po_files:
		thunar = parse_po('thunar/%s' % po_file, CleanPoFile)
	else:
		continue # If thunar doesn't translate it, I doubt Places is expected to

	if po_file in gnome_po_files:
		gnome = parse_po('gnome-panel/%s' % po_file, CleanPoFile)
		gnome.replace_id('Desktop Folder|Desktop', 'Desktop')
	else:
		gnome = PoFile()
	
		
	po, (misses, strings) = priority_make_po(pot, [thunar.intersect(pot), gnome.intersect(pot)])

	print '%s [%s] strings translated' % (po_file.ljust(20), ('X'*(strings-misses)).ljust(strings))
	po.to_file('../po/%s' % po_file)
	
	linguas.append(po_file[:-3]) # strip off .po

linguas.sort()

linguas_file = open('../po/LINGUAS', 'w')
linguas_file.write('# set of available languages (in alphabetic order)\n')
linguas_file.write(' '.join(linguas))
linguas_file.close()
