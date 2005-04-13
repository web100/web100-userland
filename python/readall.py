"""readall.py: Read all Web100 variables from all connections."""

from Web100 import *

a = Web100Agent()
cl = a.all_connections()

for c in cl:
	print("Connection %d (%s %d  %s %d)"%(c.cid, \
	                                      c.read('LocalAddress'), \
					      c.read('LocalPort'), \
					      c.read('RemAddress'), \
					      c.read('RemPort')))
	for (name, val) in c.readall().items():
		print("%-20s %s"%(name, str(val)))
	print('')
