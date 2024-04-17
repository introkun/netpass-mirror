from http.server import BaseHTTPRequestHandler, HTTPServer
from raw_types import RawMessage, MAX_MESSAGE_SIZE
import sqlite3, math, time, struct, base64

HOST = "0.0.0.0"
PORT = 8080

def get_current_timestamp():
	now = time.localtime()
	return struct.pack('<I6BH', now.tm_year, now.tm_mon, now.tm_mday, now.tm_wday, now.tm_hour, now.tm_min, now.tm_sec, 0)

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
		COMMIT;
		""")
		self.con.commit()
	def store_outbox(self, mac, msg):
		self.cur.execute("DELETE FROM outbox WHERE title_id = ? AND mac = ?",
			(msg.title_id, mac))
		self.cur.execute("INSERT INTO outbox (title_id, message_id, mac, message, time) VALUES (?, ?, ?, ?, ?)",
			(msg.title_id, msg.message_id, mac, msg.data, math.floor(time.time())))
		self.con.commit()
	def update_inbox(self, mac, title_id):
		res = self.cur.execute("SELECT message FROM outbox WHERE title_id = ? AND mac = ? LIMIT 1", (title_id, mac))
		res = res.fetchone()
		if not res:
			return False
		ownmsg = RawMessage(res[0])
		ownmsg.ts_sent = get_current_timestamp()
		res = self.cur.execute("SELECT title_id, message_id, mac, message FROM outbox WHERE title_id = ? AND mac <> ? ORDER BY time DESC LIMIT 10", (title_id, mac))
		data = []
		for row in res.fetchall():
			(title_id, message_id, from_mac, message) = row
			msg = RawMessage(message)
			msg.ts_sent = get_current_timestamp()
			msg.message_id2 = ownmsg.message_id
			ownmsg.message_id2 = msg.message_id
			to_mac = mac
			data.append((title_id, message_id, from_mac, to_mac, msg.data, math.floor(time.time())))
			data.append((title_id, ownmsg.message_id, to_mac, from_mac, ownmsg.data, math.floor(time.time())))
		for d in data:
			try:
				self.cur.execute("INSERT INTO inbox (title_id, message_id, from_mac, to_mac, message, time) VALUES (?, ?, ?, ?, ?, ?)", d)
				self.con.commit()
			except sqlite3.IntegrityError:
				pass
		return True
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
	def cleanup(self):
		self.cur.execute("DELETE FROM outbox WHERE time < ?", (math.floor(time.time() - 60*60*24*30),))
		self.cur.execute("DELETE FROM inbox WHERE time < ?", (math.floor(time.time() - 60*60*24*90),))

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
		# aaaaand update those streetpasses
		database.update_inbox(mac, msg.title_id)
		self.write_response(200, "Success")
		database.cleanup()
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
		database.cleanup()
	def do_GET(self):
		if self.path.startswith("/inbox"):
			parts = self.path.split("/")
			if len(parts) > 2:
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
						print(title_id)
						return self.pop_inbox(title_id)
		self.write_response(404, "path not found")
	def do_POST(self):
		if self.path == "/outbox/upload":
			return self.upload_new_messages()
		self.write_response(404, "Path not found")

if __name__ == "__main__":
	database = Database()
	web_server = HTTPServer((HOST, PORT), StreetPassServer)
	print(f'Started server on http://{HOST}:{PORT}')

	try:
		web_server.serve_forever()
	except KeyboardInterrupt:
		pass
	web_server.server_close()
	print("Server stopped")