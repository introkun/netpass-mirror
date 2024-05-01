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

import psycopg2, math, time, random, struct, itertools, threading
from raw_types import RawMessage

def get_current_timestamp():
	now = time.localtime()
	return struct.pack('<I6BH', now.tm_year, now.tm_mon, now.tm_mday, now.tm_wday, now.tm_hour, now.tm_min, now.tm_sec, 0)

REASON_ID_SEND_METHOD = 1
REASON_ID_SEND_COUNT = 2
REASON_ID_FORWARD_COUNT = 3

class Database:
	def __init__(self, config):
		self.config = config
		with self.con().cursor() as cur:
			cur.execute("""
			BEGIN;
			CREATE TABLE IF NOT EXISTS outbox (
				title_id INT NOT NULL,
				message_id BIGINT NOT NULL,
				mac BIGINT NOT NULL,
				message BYTEA NOT NULL,
				time BIGINT NOT NULL,
				send_count INT NOT NULL,
				modified BOOL NOT NULL DEFAULT false
			);
			CREATE INDEX IF NOT EXISTS outbox_title_id ON outbox (title_id);
			DROP INDEX IF EXISTS outbox_message_id;
			CREATE UNIQUE INDEX IF NOT EXISTS outbox_unique_message_id ON outbox (mac, message_id);
			CREATE INDEX IF NOT EXISTS outbox_mac ON outbox (mac);
			ALTER TABLE outbox ADD COLUMN IF NOT EXISTS send_count INTEGER NOT NULL DEFAULT 255;
			ALTER TABLE outbox ADD COLUMN IF NOT EXISTS modified BOOL NOT NULL DEFAULT false;
			ALTER TABLE outbox ALTER COLUMN send_count DROP DEFAULT;


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

			CREATE TABLE IF NOT EXISTS mboxlist (
				mac BIGINT NOT NULL,
				title_id INT NOT NULL,
				time BIGINT NOT NULL
			);
			CREATE INDEX IF NOT EXISTS mboxlist_mac ON mboxlist (title_id);
			CREATE UNIQUE INDEX IF NOT EXISTS mboxlist_unique ON mboxlist (mac, title_id);

			CREATE TABLE IF NOT EXISTS research (
				title_id INT NOT NULL,
				reason TEXT NOT NULL,
				reason_id INT NOT NULL,
				count INT NOT NULL DEFAULT 1
			);
			CREATE UNIQUE INDEX IF NOT EXISTS research_unique ON research (title_id, reason_id);
			ALTER TABLE research ADD COLUMN IF NOT EXISTS count INT NOT NULL DEFAULT 1;

			CREATE TABLE IF NOT EXISTS title (
				title_id INT NOT NULL,
				title_name VARCHAR(50) NOT NULL,
				count INT NOT NULL DEFAULT 1
			);
			CREATE UNIQUE INDEX IF NOT EXISTS title_unique ON title (title_id, title_name);
			COMMIT;
			""")
		self.con().commit()
	connections = {}
	def con(self):
		thread_id = threading.get_ident()
		if thread_id in self.connections:
			return self.connections[thread_id]
		self.connections[thread_id] = psycopg2.connect(
			user=self.config.get("postgres.user"),
			password=self.config.get("postgres.password"),
			host=self.config.get("postgres.host"),
			port=self.config.get("postgres.port"),
			database=self.config.get("postgres.database")
		)
		return self.connections[thread_id]

	def store_research(self, cur, title_id, msg, research_id):
		cur.execute("""
		INSERT INTO research AS r (title_id, reason, reason_id) VALUES (%s, %s, %s)
		ON CONFLICT (title_id, reason_id) DO UPDATE SET count = r.count + 1
		""", (title_id, msg, research_id))

	def store_mboxlist(self, mac, mboxlist):
		try:
			with self.con().cursor() as cur:
				cur.execute("DELETE FROM mboxlist WHERE mac = %s", (mac,))
				for title_id in mboxlist.title_ids:
					cur.execute("INSERT INTO mboxlist (mac, title_id, time) VALUES (%s, %s, %s)", (mac, title_id, math.floor(time.time())))
		except Exception as e:
			print(e)
		finally:
			self.con().commit()

	def store_outbox(self, mac, msg, title_name):
		ret = None
		try:
			with self.con().cursor() as cur:
				cur.execute("SELECT message FROM outbox WHERE title_id = %s AND mac = %s AND message_id = %s AND modified = true", (msg.title_id, mac, msg.message_id))
				res = cur.fetchone()
				if res is not None:
					oldmsg = RawMessage(res[0])
					if oldmsg.send_count < msg.send_count:
						ret = oldmsg
						msg.send_count = oldmsg.send_count
				curtime = math.floor(time.time())
				if title_name is not None:
					cur.execute("""
					INSERT INTO title AS t (title_id, title_name) VALUES (%s, %s)
					ON CONFLICT (title_id, title_name) DO UPDATE SET count = t.count + 1
					""", (msg.title_id, title_name))
				if msg.send_count == 0:
					cur.execute("DELETE FROM outbox WHERE title_id = %s AND message_id = %s AND mac = %s", (msg.title_id, msg.message_id, mac))
					return ret
				cur.execute("""
				INSERT INTO outbox
				(title_id, message_id, mac, message, time, send_count) VALUES (%s, %s, %s, %s, %s, %s)
				ON CONFLICT (mac, message_id) DO UPDATE SET
					message = %s, time = %s, send_count = %s, modified = false
				""", (msg.title_id, msg.message_id, mac, msg.data, curtime, msg.send_count, msg.data, curtime, msg.send_count))
				if msg.send_method not in (0, 1, 3):
					self.store_research(msg.title_id, f'Unknown msg send_method {msg.send_method}', REASON_ID_SEND_METHOD)
				if msg.send_count not in (0, 1, 0xFF):
					self.store_research(msg.title_id, f'Interesting send_count {msg.send_count}', REASON_ID_SEND_COUNT)
				if msg.forward_count not in (1,):
					self.store_research(msg.title_id, f'Interesting forward_count {msg.forward_count}', REASON_ID_FORWARD_COUNT)
			return ret
		finally:
			self.con().commit()
	def pop_inbox(self, mac, title_id):
		try:
			with self.con().cursor() as cur:
				cur.execute("SELECT time, message_id, from_mac, to_mac, message FROM inbox WHERE title_id = %s AND to_mac = %s AND sent = false ORDER BY time DESC LIMIT 1", (title_id, mac))
				res = cur.fetchone()
				if res is None:
					return None
				msg = RawMessage(res[4])
				cur.execute("UPDATE inbox SET sent = true WHERE message_id = %s AND to_mac = %s", (msg.message_id, res[3]))
				if not msg.validate(): return None
				return (msg, res[2])
		finally:
			self.con().commit()
	def enter_location(self, mac, location_id):
		try:
			with self.con().cursor() as cur:
				if type(location_id) is not int or location_id < 0 or location_id >= self.config.get("num_locations"):
					return False
				cur.execute("SELECT count(*) FROM mboxlist WHERE mac = %s", (mac,))
				res = cur.fetchone()
				if res is None or res[0] == 0:
					return False
				cur.execute("INSERT INTO location (location_id, mac, time_start, time_end) VALUES (%s, %s, %s, %s) ON CONFLICT DO NOTHING", (location_id, mac, math.floor(time.time()), math.floor(time.time() + 60*60*10)))
				return True
		finally:
			self.con().commit()
	def get_location(self, mac):
		try:
			with self.con().cursor() as cur:
				cur.execute("SELECT location_id FROM location WHERE mac = %s", (mac,))
				res = cur.fetchone()
				if res is None:
					return -1
				return res[0]
		finally:
			self.con().commit()
	def streetpass_location(self, mac, location_id):
		try:
			with self.con().cursor() as cur:
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
			self.con().commit()
	def streetpass_location_bg(self, location_id):
		try:
			with self.con().cursor() as cur:
				cur.execute("SELECT COUNT(*) FROM location WHERE location_id = %s", (location_id,))
				population = cur.fetchone()[0]
				limit = math.ceil(min(1, population / 1000) * population)
				cur.execute("""
				SELECT l1.mac, (
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
			self.con().commit()
	def streetpass_mac(self, mac1, mac2):
		if mac1 == mac2:
			return False
		try:
			with self.con().cursor() as cur:
				cur.execute("""
				SELECT any_value(m.mac), any_value(o.mac), any_value(o.message)
				FROM outbox o INNER JOIN mboxlist m ON o.title_id = m.title_id
				WHERE m.mac = %s AND o.mac = %s AND o.send_count <> 0
				GROUP BY o.title_id
				""", (mac1, mac2))
				rows = []
				rows.append(cur.fetchall())
				cur.execute("""
				SELECT any_value(m.mac), any_value(o.mac), any_value(o.message)
				FROM outbox o INNER JOIN mboxlist m ON o.title_id = m.title_id
				WHERE m.mac = %s AND o.mac = %s AND o.send_count <> 0
				GROUP BY o.title_id
				""", (mac2, mac1))
				rows.append(cur.fetchall())
				passes = []
				for i in range(2):
					p = {}
					for r in rows[i]:
						(mac_recv, mac_send, buf) = r
						msg = RawMessage(buf)
						if msg.send_count <= 0:
							continue
						p[msg.title_id] = (mac_recv, mac_send, msg)
					passes.append(p)
				for i in range(2):
					for title_id, r in passes[i].items():
						(mac_recv, mac_send, msg) = r
						if msg.send_count <= 0:
							continue
						
						if msg.send_method == 0:
							# we are required to have a bi-directional pass
							if title_id in passes[1 - i]:
								msg.message_id2 = passes[1 - i][title_id][2].message_id
							else:
								continue
						if msg.send_count < 0xFF:
							msg.send_count -= 1
							cur.execute("UPDATE outbox SET send_count = %s, message = %s, modified = true WHERE mac = %s AND message_id = %s",
								(msg.send_count, msg.data, mac_send, msg.message_id))
						msg.forward_count -= 1
						msg.ts_sent = get_current_timestamp()
						
						cur.execute("INSERT INTO inbox (title_id, message_id, from_mac, to_mac, message, time) VALUES (%s, %s, %s, %s, %s, %s) ON CONFLICT DO NOTHING",
							(msg.title_id, msg.message_id, mac_send, mac_recv, msg.data, math.floor(time.time())))
				return True
		finally:
			self.con().commit()
	def cleanup(self):
		try:
			with self.con().cursor() as cur:
				curtime = math.floor(time.time())
				cur.execute("DELETE FROM outbox WHERE time < %s", (curtime - 60*60*24*30,))
				cur.execute("DELETE FROM inbox WHERE time < %s", (curtime - 60*60*24*30,))
				cur.execute("DELETE FROM mboxlist WHERE time < %s", (curtime - 60*60*24*30,))
				cur.execute("DELETE FROM location WHERE time_end < %s", (curtime,))
		finally:
			self.con().commit()
