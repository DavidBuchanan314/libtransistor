import socketserver
import struct
import os
import random

PKT_HEARTBEAT = 0x00
PKT_OPEN      = 0x01
PKT_CLOSE     = 0x02
PKT_READ      = 0x03
PKT_WRITE     = 0x04
PKT_LSEEK     = 0x05
PKT_OPENDIR   = 0x06
PKT_READDIR   = 0x07

class FsNetTCPHandler(socketserver.BaseRequestHandler):
	def handle(self):
		self.fs_root = b"/tmp/fs/"
		self.dirs = {}
		print("New connection.")
		while self.handle_fsnet_packet():
			pass
		print("Connection closed.")
		self.request.close()
	
	def translate_flags(self, flags):
		wronly = bool(flags & 0x001)
		rdwr   = bool(flags & 0x002)
		append = bool(flags & 0x008)
		creat  = bool(flags & 0x200)
		trunc  = bool(flags & 0x400)
		
		translated = (append << 10) | (trunc << 9) | (creat << 6) | (rdwr << 1) | wronly
		#print("Translated flags 0x%08X" % translated)
		return translated
	
	def handle_fsnet_packet(self):
		pkt_type = self.request.recv(1)
		if not pkt_type:
			return False
		
		pkt_type = pkt_type[0]
		
		if pkt_type == PKT_HEARTBEAT:
			print("heartbeat()")
			self.request.send(self.request.recv(4))
			
		elif pkt_type == PKT_OPEN:
			flags, mode, name_len = struct.unpack("<IIB", self.request.recv(4+4+1))
			filename = self.fs_root + self.request.recv(name_len)
			try:
				result = os.open(filename, self.translate_flags(flags), mode)
			except Exception as e:
				print(e)
				result = -1
			print("open(%s, 0x%08x, 0x%08x) = %d" % (filename, flags, mode, result))
			self.request.send(struct.pack("<i", result))
		
		elif pkt_type == PKT_CLOSE:
			fd = struct.unpack("<i", self.request.recv(4))[0]
			try:
				os.close(fd)
				result = 0
			except Exception as e:
				print(e)
				result = -1
			print("close(%d) = %d" % (fd, result))
			self.request.send(struct.pack("<i", result))
		
		elif pkt_type == PKT_READ:
			fd, length = struct.unpack("<iQ", self.request.recv(4+8))
			try:
				result = os.read(fd, length)
				result_len = len(result)
			except Exception as e:
				print(e)
				result = b""
				result_len = -1;
			print("read(%d, %u) = %d" % (fd, length, result_len))
			self.request.send(struct.pack("<q", result_len) + result)
		
		elif pkt_type == PKT_WRITE:
			fd, length = struct.unpack("<iQ", self.request.recv(4+8))
			len_remaining = length
			data = b""
			while len_remaining > 0:
				data += self.request.recv(len_remaining)
				len_remaining = length - len(data)
			try:
				result = os.write(fd, data)
			except Exception as e:
				print(e)
				result = -1
			print("write(%d, buf, %u) = %d" % (fd, length, result))
			self.request.send(struct.pack("<q", result))
		
		elif pkt_type == PKT_LSEEK:
			fd, offset, whence = struct.unpack("<iii", self.request.recv(4+4+4))
			try:
				result = os.lseek(fd, offset, whence)
			except Exception as e:
				print(e)
				result = -1
			self.request.send(struct.pack("<q", result))
		
		elif pkt_type == PKT_OPENDIR:
			name_len = struct.unpack("<B", self.request.recv(1))[0]
			dirname = self.fs_root + self.request.recv(name_len)
			try:
				scanner = os.scandir(dirname)
				index = random.randint(0, 0x7FFFFFFF) # XXX collisions are possible
				self.dirs[index] = scanner
			except Exception as e:
				print(e)
				index = -1
			print("opendir(%s) = %u" % (dirname, index))
			self.request.send(struct.pack("<i", index))
		
		elif pkt_type == PKT_READDIR:
			index = struct.unpack("<i", self.request.recv(4))[0]
			try:
				dirname = next(self.dirs[index]).name
			except Exception as e:
				print(e)
				dirname = b""
			print("readdir(%d) = %s" % (index, dirname))
			self.request.send(struct.pack("<B", len(dirname)) + dirname)
		
		else:
			print("Unknown packet type! 0x%02X" % pkt_type)
			return False
		
		return True

if __name__ == "__main__":
	HOST, PORT = "0.0.0.0", 1337
	server = socketserver.TCPServer((HOST, PORT), FsNetTCPHandler)
	server.serve_forever()
