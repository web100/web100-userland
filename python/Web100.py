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


def make_vardict(_group):
	vars = {}
	_cur = libweb100.web100_var_head(_group)
	while _cur != None:
		var = Web100Var(_cur, _group)
		vars[str(var)] = var
		_cur = libweb100.web100_var_next(_cur)
	return vars


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
		self._read_group = libweb100.web100_group_find(_agent, "read")
		if self._read_group == None:
			libweb100_err()
		
		self.write_vars = make_vardict(self._tune_group)
		self.read_vars = make_vardict(self._read_group)
		for (name, var) in self.write_vars.items():
			try:
				self.read_vars[name]
			except:
				self.read_vars[name] = var
		
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
		var.valtobuf(val, self.agent.bufp)
		if libweb100.web100_raw_write(var._var, self._connection, self.agent.bufp) != \
		   libweb100.WEB100_ERR_SUCCESS:
			libweb100_err()


class Web100ReadLog:
	def __init__(self, logname):
		self._log = libweb100.web100_log_open_read(logname)
		if self._log == None:
			libweb100_err()
		self._snap = libweb100.web100_snapshot_alloc_from_log(self._log)
		if self._snap == None:
			libweb100_err()
		
		self.vars = make_vardict(libweb100.web100_get_log_group(self._log))
		
		self.bufp = libweb100.new_bufp()
	
	def __del__(self):
		libweb100.delete_bufp(self.bufp)
	
	def read(self):
		if libweb100.web100_snap_from_log(self._snap, self._log) != \
		   libweb100.WEB100_ERR_SUCCESS:
			return None
		snap = {}
		for (name, var) in self.vars.items():
			if libweb100.web100_snap_read(var._var, self._snap, self.bufp) != \
			   libweb100.WEB100_ERR_SUCCESS:
				libweb100_err()
			snap[name] = var.val(self.bufp)
		return snap


class Web100WriteLog:
	def __init__(self, logname, conn, _snap):
		self.conn = conn
		self._snap = _snap
		self._log = libweb100.web100_log_open_write(logname, conn._connection, libweb100.web100_get_snap_group(_snap))
		if self._log == None:
			libweb100_err()
	
	def write(self):
		if libweb100.web100_log_write(self._log, self._snap) != \
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
		elif self._type == libweb100.WEB100_TYPE_INET_ADDRESS_IPV4 or \
		     self._type == libweb100.WEB100_TYPE_IP_ADDRESS or \
		     self._type == libweb100.WEB100_TYPE_INET_ADDRESS_IPV6 or \
		     self._type == libweb100.WEB100_TYPE_INET_ADDRESS:
			return libweb100.web100_value_to_text(self._type, bufp)
		else:
			raise error("Unknown Web100 type: %d"%self._type)
	
	def valtobuf(self, val, bufp):
		if self._type == libweb100.WEB100_TYPE_INTEGER or \
		   self._type == libweb100.WEB100_TYPE_INTEGER32:
			libweb100.s32p_assign(libweb100.bufp_to_s32p(bufp), val)
		elif self._type == libweb100.WEB100_TYPE_GAUGE32 or \
		     self._type == libweb100.WEB100_TYPE_UNSIGNED32:
			libweb100.u32p_assign(libweb100.bufp_to_u32p(bufp), val)
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
