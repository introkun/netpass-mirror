import psycopg2, math, time, random, struct
from raw_types import RawMessage

def get_current_timestamp():
	now = time.localtime()
	return struct.pack('<I6BH', now.tm_year, now.tm_mon, now.tm_mday, now.tm_wday, now.tm_hour, now.tm_min, now.tm_sec, 0)


class Database:
	def __init__(self, config):
		self.config = config
		self.con = connection = psycopg2.connect(
			user=config.get("postgres.user"),
			password=config.get("postgres.password"),
			host=config.get("postgres.host"),
			port=config.get("postgres.port"),
			database=config.get("postgres.database")
		)
		with self.con.cursor() as cur:
			cur.execute("""
			BEGIN;
			CREATE TABLE IF NOT EXISTS outbox (
				title_id INT NOT NULL,
				message_id BIGINT NOT NULL,
				mac BIGINT NOT NULL,
				message BYTEA NOT NULL,
				time BIGINT NOT NULL
			);
			CREATE INDEX IF NOT EXISTS outbox_title_id ON outbox (title_id);
			CREATE INDEX IF NOT EXISTS outbox_message_id ON outbox (message_id);
			CREATE INDEX IF NOT EXISTS outbox_mac ON outbox (mac);

			CREATE TABLE IF NOT EXISTS inbox (
				title_id INT NOT NULL,
				message_id BIGINT NOT NULL,
				from_mac BIGINT NOT NULL,
				to_mac BIGINT NOT NULL,
				message BYTEA NOT NULL,
				sent BOOL NOT NULL DEFAULT false,
				time BIGINT NOT NULL
			);
			CREATE INDEX IF NOT EXISTS inbox_title_id ON inbox (title_id);
			CREATE INDEX IF NOT EXISTS inbox_message_id ON inbox (message_id);
			CREATE INDEX IF NOT EXISTS inbox_from_mac ON inbox (from_mac);
			CREATE INDEX IF NOT EXISTS inbox_to_mac ON inbox (to_mac);
			CREATE UNIQUE INDEX IF NOT EXISTS inbox_unique ON inbox (message_id, to_mac);

			CREATE TABLE IF NOT EXISTS location (
				location_id INT NOT NULL,
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
		with self.con.cursor() as cur:
			cur.execute("DELETE FROM outbox WHERE title_id = %s AND mac = %s",
				(msg.title_id, mac))
			cur.execute("INSERT INTO outbox (title_id, message_id, mac, message, time) VALUES (%s, %s, %s, %s, %s)",
				(msg.title_id, msg.message_id, mac, msg.data, math.floor(time.time())))
		self.con.commit()
	def pop_inbox(self, mac, title_id):
		try:
			with self.con.cursor() as cur:
				cur.execute("SELECT title_id, message_id, from_mac, to_mac, message FROM inbox WHERE title_id = %s AND to_mac = %s AND sent = false ORDER BY time DESC LIMIT 1", (title_id, mac))
				res = cur.fetchone()
				if res is None:
					return None
				msg = RawMessage(res[4])
				cur.execute("UPDATE inbox SET sent = true WHERE message_id = %s AND to_mac = %s", (msg.message_id, res[3]))
				if not msg.validate(): return None
				return (msg, res[2])
		finally:
			self.con.commit()
	def enter_location(self, mac, location_id):
		try:
			with self.con.cursor() as cur:
				if type(location_id) is not int or location_id < 0 or location_id >= self.config.get("num_locations"):
					return False
				cur.execute("INSERT INTO location (location_id, mac, time_start, time_end) VALUES (%s, %s, %s, %s) ON CONFLICT DO NOTHING", (location_id, mac, math.floor(time.time()), math.floor(time.time() + 60*60*10)))
				return True
		finally:
			self.con.commit()
	def get_location(self, mac):
		try:
			with self.con.cursor() as cur:
				cur.execute("SELECT location_id FROM location WHERE mac = %s", (mac,))
				res = cur.fetchone()
				if res is None:
					return -1
				return res[0]
		finally:
			self.con.commit()
	def streetpass_location(self, mac, location_id):
		try:
			with self.con.cursor() as cur:
				cur.execute("SELECT time_start, time_end FROM location WHERE mac = %s AND location_id = %s", (mac, location_id))
				res = cur.fetchone()
				curtime = math.floor(time.time())
				if res is None or res[0] > curtime or res[1] < curtime:
					return False
				cur.execute("SELECT mac FROM location WHERE mac <> %s AND location_id = %s ORDER BY RANDOM() LIMIT %s",
					(mac, location_id, random.randint(1, 3)))
				for row in cur.fetchall():
					self.streetpass_mac(mac, row[0])
				return True
		finally:
			self.con.commit()
	def streetpass_location_bg(self, location_id):
		try:
			with self.con.cursor() as cur:
				cur.execute("SELECTL COUNT(*) FROM location WHERE location_id = %s", (location_id,))
				population = cur.fetchone()[0]
				limit = math.ceil(min(1, population / 1000) * population)
				cur.execute("""
				SELECT l1.mac, (j
					SELECT l2.mac
					FROM location l2
					WHERE l2.location_id = l1.location_id AND l1.mac <> l2.mac
					ORDER BY RANDOM() LIMIT 1
				) mac
				FROM location l1
				WHERE l1.location_id = %s
				ORDER BY RANDOM() LIMIT %s
				""", (location_id, limit))
				for row in cur.fetchall():
					self.streetpass_mac(row[0], row[1])
		finally:
			self.con.commit()
	def streetpass_mac(self, mac1, mac2):
		if mac1 == mac2:
			return False
		try:
			with self.con.cursor() as cur:
				cur.execute("""
				SELECT o1.title_id, o1.mac, o1.message, o2.mac, o2.message
				FROM outbox o1 INNER JOIN outbox o2 ON o1.title_id = o2.title_id
				WHERE o1.mac = %s AND o2.mac = %s
				""", (mac1, mac2))
				curtime = math.floor(time.time())
				data = []
				for row in cur.fetchall():
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
					cur.execute("INSERT INTO inbox (title_id, message_id, from_mac, to_mac, message, time) VALUES (%s, %s, %s, %s, %s, %s) ON CONFLICT DO NOTHING", d)
				return True
		finally:
			self.con.commit()
	def cleanup(self):
		try:
			with self.con.cursor() as cur:
				curtime = math.floor(time.time())
				cur.execute("DELETE FROM outbox WHERE time < %s", (curtime - 60*60*24*30,))
				cur.execute("DELETE FROM inbox WHERE time < %s", (curtime - 60*60*24*90,))
				cur.execute("DELETE FROM location WHERE time_end < %s", (curtime,))
		finally:
			self.con.commit()
