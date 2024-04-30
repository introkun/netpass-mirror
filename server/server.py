from http.server import BaseHTTPRequestHandler, HTTPServer
from raw_types import RawMessage, MAX_MESSAGE_SIZE, MBOXLIST_SIZE, MBoxList
from threading import Timer
from database import Database
from config import Config
from socketserver import ThreadingMixIn
import struct, traceback

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
		# now we have to store the new outbox message
		newmsg = database.store_outbox(mac, msg)
		if not newmsg:
			return self.write_response(204, "Success")
		self.send_response(200)
		self.send_header("Content-Typ", "application/binary")
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
		self.send_header("Content-Typ", "application/binary")
		#self.send_header("3ds-mac", struct.pack('<q', from_mac).hex()[0:12])
		self.end_headers()
		self.wfile.write(msg.data)
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
		except Exception as e:
			print(e)
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
			self.write_response(404, "path not found")
		except Exception as e:
			print(e)
			print(traceback.format_exc())
			self.write_response(500, "Internal Server Error")
	def do_POST(self):
		try:
			if self.path == "/outbox/mboxlist":
				return self.upload_mboxlist()
			if self.path == "/outbox/upload":
				return self.upload_new_messages()
			self.write_response(404, "Path not found")
		except Exception as e:
			print(e)
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