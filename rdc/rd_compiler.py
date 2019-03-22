#!/usr/bin/env python

import sys, os

keyword = {
	'do_step' : 1,
	'do_steps' : 2,
	'turn_left' : 3,
	'turn_right' : 4,
	'radar_ping' : 5,
	'fire_rocket' : 6,
	'return' : 16,
	'$end' : 62,
	'@end' : 62,

	'goto' : 64,
}
labels = {
	'$begin' : 0,
}

lpc = 0
lcnt = 0
skip = 1


if (len(sys.argv) == 2): 
	fname = sys.argv[1]
else:
	fname = 'test1.rdc'
	
f = open(fname)
bf = open(fname+'.com', 'w+')

for line in f:
	lcnt += 1
	kw = line.strip().split('//')[0]
	if (kw == ''):
		continue
	if (skip):
		if (kw == '$begin'):
			skip = 0
			continue
	
	if (':' in kw) :
		lab = kw.strip(':')
		print "Add label: "+ lab
		labels[lab] = lpc
		if (lab[0] == '@'):
			bf.write('\n')
		continue
		
	lkey = kw.split()[0]
	if lkey in keyword :
		lpc += 1
		#print "KW:"+kw
		if (keyword[lkey] > 63):
			par = kw.split()[1]
			bf.write(str(keyword[lkey])+' ')
			bf.write(str(labels[par])+'|')
		else :
			bf.write(str(keyword[lkey])+'|')
	else :
		print 'COMPILE ERROR: Unknown instuction >>' + kw + '<< in line number ' + str(lcnt)
	
f.close()
bf.close()

print "Compiled %d lines of code to %d instruction" % (lcnt, lpc)
#print labels