from http.server import BaseHTTPRequestHandler, HTTPServer
from raw_types import RawMessage, MAX_MESSAGE_SIZE
from threading import Timer
import sqlite3, math, time, struct, base64, random

NUM_LOCATIONS = 3

HOST = "0.0.0.0"
PORT = 8080

def get_current_timestamp():
	now = time.localtime()
	return struct.pack('<I6BH', now.tm_year, now.tm_mon, now.tm_mday, now.tm_wday, now.tm_hour, now.tm_min, now.tm_sec, 0)

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
			self._timer = Timer(self.interval, self._run)
			self._timer.start()
			self.is_running = True

	def stop(self):
		self._timer.cancel()
		self.is_running = False

class Database:
	def __init__(self):
		self.con = sqlite3.connect("database.db")
		self.cur = self.con.cursor()
		self.cur.executescript("""
		BEGIN;
		CREATE TABLE IF NOT EXISTS outbox (
			title_id INTIGER NOT NULL,
			message_id BIGINT NOT NULL,
			mac BIGINT NOT NULL,
			message BLOB NOT NULL,
			time BIGINT NOT NULL
		);
		CREATE INDEX IF NOT EXISTS outbox_title_id ON outbox (title_id);
		CREATE INDEX IF NOT EXISTS outbox_message_id ON outbox (message_id);
		CREATE INDEX IF NOT EXISTS outbox_mac ON outbox (mac);

		CREATE TABLE IF NOT EXISTS inbox (
			title_id INTIGER NOT NULL,
			message_id BIGINT NOT NULL,
			from_mac BIGINT NOT NULL,
			to_mac BIGINT NOT NULL,
			message BLOB NOT NULL,
			sent INTIGER NOT NULL DEFAULT 0,
			time BIGINT NOT NULL
		);
		CREATE INDEX IF NOT EXISTS inbox_title_id ON inbox (title_id);
		CREATE INDEX IF NOT EXISTS inbox_message_id ON inbox (message_id);
		CREATE INDEX IF NOT EXISTS inbox_from_mac ON inbox (from_mac);
		CREATE INDEX IF NOT EXISTS inbox_to_mac ON inbox (to_mac);
		CREATE UNIQUE INDEX IF NOT EXISTS inbox_unique ON inbox (message_id, to_mac);

		CREATE TABLE IF NOT EXISTS location (
			location_id INTIGER NOT NULL,
			mac BIGINT NOT NULL,
			time_start BIGINT NOT NULL,
			time_end BIGINT NOT NULL
		);
		CREATE INDEX IF NOT EXISTS location_location_id ON location (location_id);
		CREATE UNIQUE INDEX IF NOT EXISTS location_mac_unique ON location (mac);
		COMMIT;
		""")
		self.con.commit()
	def store_outbox(self, mac, msg):
		self.cur.execute("DELETE FROM outbox WHERE title_id = ? AND mac = ?",
			(msg.title_id, mac))
		self.cur.execute("INSERT INTO outbox (title_id, message_id, mac, message, time) VALUES (?, ?, ?, ?, ?)",
			(msg.title_id, msg.message_id, mac, msg.data, math.floor(time.time())))
		self.con.commit()
	def pop_inbox(self, mac, title_id):
		res = self.cur.execute("SELECT title_id, message_id, from_mac, to_mac, message FROM inbox WHERE title_id = ? AND to_mac = ? AND sent = 0 ORDER BY time DESC LIMIT 1", (title_id, mac))
		res = res.fetchone()
		if res is None:
			return None
		msg = RawMessage(res[4])
		self.cur.execute("UPDATE inbox SET sent = 1 WHERE message_id = ? AND to_mac = ?", (msg.message_id, res[3]))
		self.con.commit()
		if not msg.validate(): return None
		return (msg, res[2])
	def enter_location(self, mac, location_id):
		if type(location_id) is not int or location_id < 0 or location_id >= NUM_LOCATIONS:
			return False
		try:
			self.cur.execute("INSERT INTO location (location_id, mac, time_start, time_end) VALUES (?, ?, ?, ?)", (location_id, mac, math.floor(time.time()), math.floor(time.time() + 60*60*10)))
			self.con.commit()
			return True
		except sqlite3.IntegrityError:
			return False
	def get_location(self, mac):
		res = self.cur.execute("SELECT location_id FROM location WHERE mac = ?", (mac,))
		res = res.fetchone()
		if res is None:
			return -1
		return res[0]
	def streetpass_location(self, mac, location_id):
		res = self.cur.execute("SELECT time_start, time_end FROM location WHERE mac = ? AND location_id = ?", (mac, location_id))
		res = res.fetchone()
		curtime = math.floor(time.time())
		if res is None or res[0] > curtime or res[1] < curtime:
			return False
		res = self.cur.execute("SELECT mac FROM location WHERE mac <> ? AND location_id = ? AND time_start < ? AND time_end > ? ORDER BY RANDOM() LIMIT ?",
			(mac, location_id, curtime, curtime, random.randint(1, 5)))
		for row in res.fetchall():
			self.streetpass_mac(mac, row[0])
		return True
	def streetpass_location_bg(self, location_id):
		res = self.cur.execute("SELECTL COUNT(*) FROM location WHERE location_id = ?", (location_id,))
		population = res.fetchone()[0]
		limit = min(1, population / 1000) * population
		res = self.cur.execute("""
		SELECT l1.mac, (
			SELECT l2.mac
			FROM location l2
			WHERE l2.location_id = l1.location_id AND l1.mac <> l2.mac
			ORDER BY RANDOM() LIMIT 1
		) mac
		FROM location l1
		WHERE l1.location_id = ?
		ORDER BY RANDOM() LIMIT ?
		""", (location_id, limit))
		for row in res.fetchall():
			self.streetpass_mac(row[0], row[1])
	def streetpass_mac(self, mac1, mac2):
		if mac1 == mac2:
			return False
		res = self.cur.execute("""
		SELECT o1.title_id, o1.mac, o1.message, o2.mac, o2.message
		FROM outbox o1 INNER JOIN outbox o2
		WHERE o1.title_id = o2.title_id AND o1.mac = ? AND o2.mac = ?
		""", (mac1, mac2))
		curtime = math.floor(time.time())
		data = []
		for row in res.fetchall():
			(title_id, mac1, message1, mac2, message2) = row
			msg1 = RawMessage(message1)
			msg2 = RawMessage(message2)
			msg1.message_id2 = msg2.message_id
			msg2.message_id2 = msg1.message_id
			msg1.ts_sent = get_current_timestamp()
			msg2.ts_sent = get_current_timestamp()
			data.append((title_id, msg1.message_id, mac1, mac2, msg1.data, curtime))
			data.append((title_id, msg2.message_id, mac2, mac1, msg2.data, curtime))
		for d in data:
			try:
				self.cur.execute("INSERT INTO inbox (title_id, message_id, from_mac, to_mac, message, time) VALUES (?, ?, ?, ?, ?, ?)", d)
				self.con.commit()
			except sqlite3.IntegrityError:
				pass
		return True
	def cleanup(self):
		curtime = math.floor(time.time())
		self.cur.execute("DELETE FROM outbox WHERE time < ?", (curtime - 60*60*24*30,))
		self.cur.execute("DELETE FROM inbox WHERE time < ?", (curtime - 60*60*24*90,))
		self.cur.execute("DELETE FROM location WHERE time_end < ?", (curtime,))
		self.con.commit()

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
		database.store_outbox(mac, msg)
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
		self.send_header("3ds-mac", struct.pack('<q', from_mac).hex()[0:12])
		self.end_headers()
		self.wfile.write(msg.data)
	def do_PUT(self):
		if self.path.startswith("/location/"):
			parts = self.path.split("/")
			location_id = None
			try:
				location_id = int(parts[2])
				if location_id < 0 or location_id >= NUM_LOCATIONS:
					return self.write_response(400, "Invalid location id")
			except:
				return self.write_response(400, "Invalid location id")
			if len(parts) > 3:
				if parts[3] == "enter":
					return self.enter_location(location_id)
		self.write_response(404, "path not found")
	def do_GET(self):
		if self.path.startswith("/inbox/"):
			parts = self.path.split("/")
			title_id = None
			try:
				title_id = struct.unpack('>I', bytes.fromhex(parts[2]))[0]
			except:
				# b64 legacy, remove at some point
				# obsolete since  17.4.2024
				try:
					b64tid = parts[2]
					while len(b64tid) % 4: b64tid += "="
					title_id = struct.unpack('<I', base64.b64decode(b64tid, b'+-'))[0]
				except:
					return self.write_response(400, "Invalid inbox id")
			if len(parts) > 3:
				if parts[3] == "pop":
					return self.pop_inbox(title_id)
		if self.path == "/location/current":
			return self.get_location()
		self.write_response(404, "path not found")
	def do_POST(self):
		if self.path == "/outbox/upload":
			return self.upload_new_messages()
		self.write_response(404, "Path not found")

def bg_tasks_frequent():
	database.cleanup()
def bg_tasks_hourly():
	# here we need to create streetpasses within a location; the larger the location the more likely
	for location_id in range(NUM_LOCATIONS):
		database.streetpass_location_bg(location_id)

if __name__ == "__main__":
	database = Database()
	web_server = HTTPServer((HOST, PORT), StreetPassServer)
	bg_frequent = RepeatedTimer(1, bg_tasks_frequent)
	bg_hourly = RepeatedTimer(60*60, bg_tasks_hourly)
	bg_frequent.start()
	bg_hourly.start()
	print(f'Started server on http://{HOST}:{PORT}')
	try:
		web_server.serve_forever()
	except KeyboardInterrupt:
		pass
	web_server.server_close()
	bg_hourly.stop()
	bg_frequent.stop()
	print("Server stopped")