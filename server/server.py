# NetPass
# Copyright (C) 2024 Sorunome
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from http.server import BaseHTTPRequestHandler, HTTPServer
from raw_types import RawMessage, MAX_MESSAGE_SIZE, MBOXLIST_SIZE, MBoxList
from threading import Timer
from database import Database
from config import Config
from socketserver import ThreadingMixIn
import datetime, struct, traceback, base64

# From https://stackoverflow.com/a/38317060
class RepeatedTimer(object):
	def __init__(self, interval, function, *args, **kwargs):
		self._timer = None
		self.interval = interval
		self.function = function
		self.args = args
		self.kwargs = kwargs
		self.is_running = False
		self.start()

	def _run(self):
		self.is_running = False
		self.start()
		self.function(*self.args, **self.kwargs)

	def start(self):
		if not self.is_running:
			self.function(*self.args, **self.kwargs)
			self._timer = Timer(self.interval, self._run)
			self._timer.start()
			self.is_running = True

	def stop(self):
		self._timer.cancel()
		self.is_running = False

class StreetPassServer(BaseHTTPRequestHandler):
	def write_response(self, httpcode, errmsg):
		self.send_response(httpcode)
		self.send_header("Content-Type", "text/plain");
		self.end_headers()
		self.wfile.write(bytes(errmsg, "utf-8"))
	def write_str(self, s):
		self.wfile.write(bytes(s, "utf-8"))
	def get_mac(self):
		mac = self.headers['3ds-mac']
		try:
			if mac == None:
				return self.write_response(400, "Missing mac")
			if len(mac) != 12:
				return self.write_response(400, "Invalid mac")
			mac = bytes.fromhex(mac)
			if len(mac) != 6:
				return self.write_response(400, "Invalid mac")
			mac = struct.unpack('<q', mac + b'\x00\x00')[0]
		except:
			return self.write_response(400, "Invalid mac")
		return mac
	def upload_new_messages(self):
		# first verify the headers needed
		length = self.headers['content-length'];
		try:
			length = int(length)
			if length < 4: #0x70:
				return self.write_response(400, "Content too short")
			if length > MAX_MESSAGE_SIZE:
				return self.write_response(413, "Content too long")
		except:
			return self.write_response(411, "Invalid content-length error")
		mac = self.get_mac()
		if mac is None: return
		MSG_HEADER_SIZE = 0x70
		buf = self.rfile.read(MSG_HEADER_SIZE)
		msg = RawMessage(buf)
		if not msg.validate_header():
			return self.write_response(400, "Bad Message Header")
		if msg.size != length:
			return self.write_response(400, "Bad Message Length");
		buf += self.rfile.read(msg.size - MSG_HEADER_SIZE)
		msg = RawMessage(buf)
		if not msg.validate():
			return self.write_response(400, "Bad Message")
		title_name = None
		try:
			title_name = self.headers['3ds-title-name']
			while len(title_name) % 4 > 0: title_name += "="
			title_buf = base64.b64decode(title_name, b'+-')
			try:
				title_name = title_buf.decode("utf-16").strip("\x00").split("\x00")[0]
			except:
				title_name = title_buf.decode("utf-8").strip("\x00").split("\x00")[0]
			if len(title_name) > 50:
				title_name = None
		except:
			title_name = None
		# now we have to store the new outbox message
		newmsg = database.store_outbox(mac, msg, title_name)
		if not newmsg:
			return self.write_response(204, "Success")
		self.send_response(200)
		self.send_header("Content-Type", "application/binary")
		self.end_headers()
		self.wfile.write(struct.pack('<B', msg.send_count))
	def upload_mboxlist(self):
		length = self.headers['content-length'];
		try:
			length = int(length)
			if length < 4:
				print("meow?")
				return self.write_response(400, "Content too short")
			if length > MBOXLIST_SIZE:
				return self.write_response(413, "Content too long")
		except:
			return self.write_response(411, "Invalid content-length error")
		mac = self.get_mac()
		if mac is None: return
		buf = self.rfile.read(MBOXLIST_SIZE)
		mboxlist = MBoxList(buf)
		if not mboxlist.validate():
			print("nuuu")
			return self.write_response(400, "Bad Message")
		database.store_mboxlist(mac, mboxlist)
		self.write_response(200, "Success")
	def enter_location(self, location_id):
		mac = self.get_mac()
		if mac is None: return
		if not database.enter_location(mac, location_id):
			return self.write_response(409, "Cannot enter location")
		database.streetpass_location(mac, location_id)
		self.write_response(200, "Success")
	def get_location(self):
		mac = self.get_mac()
		if mac is None: return
		res = database.get_location(mac)
		if res == -1:
			return self.write_response(204, "Not in any location")
		self.send_response(200)
		self.send_header("Content-Type", "application/binary");
		self.end_headers()
		self.wfile.write(struct.pack('<i', res))
	def pop_inbox(self, title_id):
		mac = self.get_mac()
		if mac is None: return
		res = database.pop_inbox(mac, title_id)
		if res is None:
			return self.write_response(204, "Inbox empty")
		(msg, from_mac) = res
		self.send_response(200)
		self.send_header("Content-Type", "application/binary")
		#self.send_header("3ds-mac", struct.pack('<q', from_mac).hex()[0:12])
		self.end_headers()
		self.wfile.write(msg.data)
	def delete_data(self):
		mac = self.get_mac()
		if mac is None: return
		database.delete_mac_data(mac)
		self.write_response(200, "Success")
	def get_data(self):
		mac = self.get_mac()
		if mac is None: return
		(outbox, inbox_from, inbox_to, location, mboxlist) = database.get_mac_data(mac)
		self.send_response(200)
		self.send_header("Content-Type", "plain/text")
		self.end_headers()
		self.write_str("NetPass data dump\n")
		self.write_str(f'Time: {datetime.datetime.now()}\n')
		self.write_str(f'Mac Address: {struct.pack('<q', mac).hex()[0:12]}\n')
		self.write_str("\n\n")
		self.write_str("Outboxes\n")
		for o in outbox:
			self.write_str(f'  title_id: {struct.pack('<I', o[0]).hex()}\n')
			self.write_str(f'  message_id: {struct.pack('<q', o[1]).hex()}\n')
			self.write_str(f'  message: {o[2].hex()}\n')
			self.write_str(f'  time: {o[3]}\n\n')
		
		def print_inbox(i):
			self.write_str(f'  title_id: {struct.pack('<I', o[0]).hex()}\n')
			self.write_str(f'  message_id: {struct.pack('<q', o[1]).hex()}\n')
			self.write_str(f'  message: {o[2].hex()}\n')
			self.write_str(f'  sent: {o[3]}\n')
			self.write_str(f'  time: {o[4]}\n\n')
		
		self.write_str("Inboxes sent to others\n")
		for i in inbox_from:
			print_inbox(i)
		self.write_str("Inboxes sent to yourself\n")
		for i in inbox_to:
			print_inbox(i)
		self.write_str("Locations\n")
		for l in location:
			self.write_str(f'  location_id: {l[0]}\n')
			self.write_str(f'  time_start: {l[1]}\n')
			self.write_str(f'  time_end: {l[2]}\n\n')
		
		self.write_str("Activated message boxes\n")
		for b in mboxlist:
			self.write_str(f'  title_id: {struct.pack('<I', b[0]).hex()}\n')
			self.write_str(f'  time: {b[1]}\n\n')
	def do_DELETE(self):
		try:
			if self.path == "/data":
				self.delete_data()
		except:
			print(traceback.format_exc())
			self.write_response(500, "Internal Server Error")
	def do_PUT(self):
		try:
			if self.path.startswith("/location/"):
				parts = self.path.split("/")
				location_id = None
				try:
					location_id = int(parts[2])
					if location_id < 0 or location_id >= config.get("num_locations"):
						return self.write_response(400, "Invalid location id")
				except:
					return self.write_response(400, "Invalid location id")
				if len(parts) > 3:
					if parts[3] == "enter":
						return self.enter_location(location_id)
			self.write_response(404, "path not found")
		except:
			print(traceback.format_exc())
			self.write_response(500, "Internal Server Error")
	def do_GET(self):
		try:
			if self.path.startswith("/inbox/"):
				parts = self.path.split("/")
				title_id = None
				try:
					title_id = struct.unpack('>I', bytes.fromhex(parts[2]))[0]
				except:
					return self.write_response(400, "Invalid inbox id")
				if len(parts) > 3:
					if parts[3] == "pop":
						return self.pop_inbox(title_id)
			if self.path == "/location/current":
				return self.get_location()
			if self.path == "/ping":
				return self.write_response(200, "pong")
			if self.path == "/data":
				return self.get_data()
			self.write_response(404, "path not found")
		except:
			print(traceback.format_exc())
			self.write_response(500, "Internal Server Error")
	def do_POST(self):
		try:
			if self.path == "/outbox/mboxlist":
				return self.upload_mboxlist()
			if self.path == "/outbox/upload":
				return self.upload_new_messages()
			self.write_response(404, "Path not found")
		except:
			print(traceback.format_exc())
			self.write_response(500, "Internal Server Error")

class ThreadingSimpleServer(ThreadingMixIn, HTTPServer):
	pass

def bg_tasks_frequent():
	database.cleanup()
def bg_tasks_hourly():
	# here we need to create streetpasses within a location; the larger the location the more likely
	for location_id in range(config.get("num_locations")):
		database.streetpass_location_bg(location_id)

if __name__ == "__main__":
	config = Config("config.yaml")
	database = Database(config)
	web_server = ThreadingSimpleServer((config.get("server.host"), config.get("server.port")), StreetPassServer)
	bg_frequent = RepeatedTimer(1, bg_tasks_frequent)
	bg_hourly = RepeatedTimer(60*60, bg_tasks_hourly)
	bg_frequent.start()
	bg_hourly.start()
	print(f'Started server on http://{config.get("server.host")}:{config.get("server.port")}')
	try:
		web_server.serve_forever()
	except KeyboardInterrupt:
		pass
	web_server.server_close()
	bg_hourly.stop()
	bg_frequent.stop()
	print("Server stopped")