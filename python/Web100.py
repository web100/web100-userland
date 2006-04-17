"""A Python module for accessing Web100 statistics.

Written by John Heffner <jheffner@psc.edu>.

This module is built on top of a SWIG interface to libweb100,
the C library from the Web100 userland package.  Before
using this module, you must first build the Web100 userland,
then the SWIG wrapper distribueted with this module.

This was written for Python 2.3, but it will probably work
with significantly older versions.

For more information on Web100, see the Web100 website:
http://www.web100.org.


    All documentation and programs in this release is copyright (c)
2004 Carnegie Mellon University, The Board of Trustees of the
University of Illinois, and University Corporation for Atmospheric
Research.  This software comes with NO WARRANTY.

    This software product, including all source code and headers, is free
software; you can redistribute it and/or modify it under the terms of the
GNU Lesser General Public License, version 2.1.

    This material is based in whole or in part on work supported by the
National Science Foundation under Grant No. 0083285. Any opinions,
findings and conclusions or recommendations expressed in this material
are those of the author(s) and do not necessarily reflect the views of
the National Science Foundation (NSF).
"""

import libweb100
import exceptions
import fnmatch
from Web100 import *

class error(exceptions.Exception):
	"""All exception thrown by this module will have this type."""
	
	def __init__(self, msg):
		self.msg = msg
	
	def __str__(self):
		return self.msg


def libweb100_err():
	raise error("libweb100: %s"%libweb100.web100_strerror(libweb100.cvar.web100_errno))


class Web100Agent:
	"""Corresponds to an SNMP agent.
	
	The only supported type now is local, which means reading directly from
	the /proc filesystem interface on the local host.  In theory, remote
	SNMP agent support could be added in the future.
	"""
	
	def __init__(self, host=None):
		if (host != None):
			raise error("Remote agents not supported.")
		_agent = libweb100.web100_attach(libweb100.WEB100_AGENT_TYPE_LOCAL, None)
		if _agent == None:
			libweb100_err()
		self._agent = _agent
		
		self._tune_group = libweb100.web100_group_find(_agent, "tune")
		if self._tune_group == None:
			libweb100_err()
		self.write_vars = {}
		_cur = libweb100.web100_var_head(self._tune_group)
		while _cur != None:
			var = Web100Var(_cur, self._tune_group)
			self.write_vars[str(var)] = var
			_cur = libweb100.web100_var_next(_cur)
		
		self.read_vars = {}
		for (name, var) in self.write_vars.items():
			self.read_vars[name] = var
		
		self._read_group = libweb100.web100_group_find(_agent, "read")
		if self._read_group == None:
			libweb100_err()
		_cur = libweb100.web100_var_head(self._read_group)
		while _cur != None:
			var = Web100Var(_cur, self._read_group)
			self.read_vars[str(var)] = var
			_cur = libweb100.web100_var_next(_cur)
		
		self.bufp = libweb100.new_bufp()
	
	def __del__(self):
		libweb100.delete_bufp(self.bufp)
	
	def all_connections(self):
		"""All current connections from this agent.
		
		Returns a list of Web100Connection objects.
		"""
		
		conns = []
		cur = libweb100.web100_connection_head(self._agent)
		while cur != None:
			conns.append(Web100Connection(self, cur))
			cur = libweb100.web100_connection_next(cur)
		return conns
	
	def connection_from_cid(self, cid):
		"""Finds a connection from a Web100 Connection ID (cid).
		
		Returns a Web100Connection object, or None.
		"""
		
		cl = self.all_connections()
		for c in cl:
			if c.cid == cid:
				return c
		return None
	
	def connection_from_fd(self, fd):
		"""Finds a connection from a file descriptor.
		
		Returns a Web100Connection object, or None.
		"""
		
		_c = libweb100.web100_connection_from_socket(self._agent, fd)
		if _c == None:
			return None
		cl = self.all_connections()
		for c in cl:
			if c._connection == _c:
				return c
		return None
	
	def connection_match(self, local_address, local_port, remote_address, remote_port):
		"""All connections matching a (partial) 4-tuple
		
		Ports are integers, addresses are strings.  Does UNIX-style pattern
		matching on address strings.
		
		Returns a list of Web100Connection objects.
		"""
		
		cl = self.all_connections()
		match = []
		for c in cl:
			if (local_port == None or c.read('LocalPort') == local_port) and \
			   (remote_port == None or c.read('RemPort') == remote_port) and \
			   (local_address == None or \
			    fnmatch.fnmatch(c.read('LocalAddress'), local_address)) and \
			   (remote_address == None or \
			    fnmatch.fnmatch(c.read('RemAddress'), remote_address)):
				match.append(c)
		return match
	
	def var_is_counter(self, varname):
		"""Determine whether variable has a counter type"""
		
		try:
			v = self.read_vars[varname]
		except:
			return False
		
		return (v._type == libweb100.WEB100_TYPE_COUNTER32 or
	                v._type == libweb100.WEB100_TYPE_COUNTER64)


class Web100Connection:
	"""Describes one connection.
	
	This object should only be created as a side-effect of calls from
	an agent.  Use this object to access the Web100 variables of a
	connection.
	"""
	
	def __init__(self, agent, _connection):
		self.agent = agent
		self._connection = _connection
		self._readsnap = libweb100.web100_snapshot_alloc(agent._read_group, _connection)
		if self._readsnap == None:
			libweb100_err()
		self._tunesnap = libweb100.web100_snapshot_alloc(agent._tune_group, _connection)
		if self._tunesnap == None:
			libweb100_err()
		self.cid = libweb100.web100_get_connection_cid(_connection)
	
	def __del__(self):
		libweb100.web100_snapshot_free(self._readsnap)
		libweb100.web100_snapshot_free(self._tunesnap)
	
	def read(self, name):
		"""Read the value of a single variable."""
		
		try:
			var = self.agent.read_vars[name]
		except KeyError:
			raise error("No reabale variable '%s' found."%name)
		if libweb100.web100_raw_read(var._var, self._connection, self.agent.bufp) != \
		   libweb100.WEB100_ERR_SUCCESS:
			libweb100_err()
		return var.val(self.agent.bufp)
	
	def readall(self):
		"""Take a snapshot of all variables from a connection.
		
		This is much more efficient than reading all variables
		individually.  Currently, for local agents, it also guarantees
		consistency between all read-only variables.
		"""
		
		if libweb100.web100_snap(self._readsnap) != libweb100.WEB100_ERR_SUCCESS:
			libweb100_err()
		if libweb100.web100_snap(self._tunesnap) != libweb100.WEB100_ERR_SUCCESS:
			libweb100_err()
		snap = {}
		for (name, var) in self.agent.read_vars.items():
			if var._group == self.agent._read_group:
				if libweb100.web100_snap_read(var._var, self._readsnap, self.agent.bufp) != \
				   libweb100.WEB100_ERR_SUCCESS:
					libweb100_err()
			else:
				if libweb100.web100_snap_read(var._var, self._tunesnap, self.agent.bufp) != \
				   libweb100.WEB100_ERR_SUCCESS:
					libweb100_err()
			val = var.val(self.agent.bufp)
			snap[name] = val
		return snap
	
	def write(self, name, val):
		"""Write a value to a single variable."""
		
		try:
			var = self.agent.write_vars[name]
		except KeyError:
			raise error("No writable variable '%s' found."%name)
		buf = var.valtobuf(val, self.agent.bufp)
		if libweb100.web100_raw_write(var._var, self._connection, buf) != \
		   libweb100.WEB100_ERR_SUCCESS:
			libweb100_err()


class Web100Var:
	"""This class should be used only internally by this module."""
	
	def __init__(self, _var, _group):
		self._var = _var
		self._group = _group
		self._type = libweb100.web100_get_var_type(self._var)
	
	def __str__(self):
		return libweb100.web100_get_var_name(self._var)
	
	def val(self, bufp):
		if self._type == libweb100.WEB100_TYPE_INET_PORT_NUMBER or\
		   self._type == libweb100.WEB100_TYPE_UNSIGNED16:
			return libweb100.u16p_value(libweb100.bufp_to_u16p(bufp))
		elif self._type == libweb100.WEB100_TYPE_INTEGER or \
		     self._type == libweb100.WEB100_TYPE_INTEGER32:
			return libweb100.s32p_value(libweb100.bufp_to_s32p(bufp))
		elif self._type == libweb100.WEB100_TYPE_COUNTER32 or \
		     self._type == libweb100.WEB100_TYPE_GAUGE32 or \
		     self._type == libweb100.WEB100_TYPE_UNSIGNED32 or \
		     self._type == libweb100.WEB100_TYPE_TIME_TICKS:
			return libweb100.u32p_value(libweb100.bufp_to_u32p(bufp))
		elif self._type == libweb100.WEB100_TYPE_COUNTER64:
			return libweb100.u64p_value(libweb100.bufp_to_u64p(bufp))
		else:
			return libweb100.web100_value_to_text(self._type, bufp)
	
	def valtobuf(self, val, bufp):
		if self._type == libweb100.WEB100_TYPE_INTEGER or \
		   self._type == libweb100.WEB100_TYPE_INTEGER32:
			libweb100.s32p_assign(libweb100.bufp_to_s32p(bufp), val)
			return bufp
		elif self._type == libweb100.WEB100_TYPE_GAUGE32 or \
		     self._type == libweb100.WEB100_TYPE_UNSIGNED32:
			libweb100.u32p_assign(libweb100.bufp_to_u32p(bufp), val)
			return bufp
		elif self._type == libweb100.WEB100_TYPE_STR32:
			return libweb100.str_to_bufp(val)
		else:
			raise error("Unknown or unwritable type: %d"%self._type)


def counter_delta(a, b):
	"""Gives a delta value between two counter values.
	
	Works with either 32-bit or 64-bit counter, but both
	arguments should be the same type.
	"""
	
	d = b - a
	if (d < 0):
		d = d + 2**32
		if (d <= 0):
			d = d + 2**64 - 2**32
	return d
