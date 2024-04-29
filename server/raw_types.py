from enum import Enum
import struct
import base64

MAX_MESSAGE_SIZE = 0x20000
MBOXLIST_SIZE = (12 + 16*24)

def decode_timestamp(ts):
	(year, month, day, weekDay, hour, minute,
		second, millisecond) = struct.unpack('<I6BH', ts)
	print(year, month, day, weekDay, hour, minute, second, millisecond)
	return

class CecBoxType(Enum):
	inbox = 0
	outbox = 1

class CecDataPathType(Enum):
	mbox_list = 1
	mbox_info = 2
	inbox_info = 3
	outbox_info = 4
	outbox_index = 5
	inbox_msg = 6
	outbox_msg = 7
	root_dir = 10
	mbox_dir = 11
	inbox_dir = 12
	outbox_dir = 13
	mbox_data = 100
	mbox_icon = 101
	mbox_title = 110,
	mbox_program_id = 150

class GeneralClass():
	data = b''
	def __init__(self, filename):
		if type(filename) is bytes or type(filename) is bytearray:
			self.load(filename)
		elif type(filename) is memoryview:
			self.load(filename.tobytes())
		else:
			self.loadf(filename)
	def loadf(self, filename):
		with open(filename, "rb") as f:
			self.load(f.read())
	def load(self, d):
		self.data = bytearray(d)
		if hasattr(self, "MAGIC"):
			magic = self.magic
			if magic != self.MAGIC:
				raise Exception("Wrong magic")
	@property
	def magic(self):
		return struct.unpack('<H', self.data[0:2])[0]

class MBoxList(GeneralClass):
	FILENAME = "MBoxList____"
	MAGIC = 0x6868
	@property
	def version(self):
		return struct.unpack('<I', self.data[4:8])[0]
	@property
	def num_boxes(self):
		return struct.unpack('<I', self.data[8:0xC])[0]
	@property
	def title_ids(self):
		ret = []
		for i in range(0, 16):
			name = self.data[0xC + i*16:0xC + (i+1)*16].decode("ascii").strip('\x00')
			if name != '':
				ret.append(int(name, 16))
		return ret
	@property
	def box_names(self):
		ret = []
		for i in range(0, 16):
			name = self.data[0xC + i*16:0xC + (i+1)*16].decode("ascii").strip('\x00')
			if name != '':
				ret.append(name)
		return ret
	def validate(self):
		try:
			if self.magic != self.MAGIC or self.num_boxes > 24 or len(self.data) > MBOXLIST_SIZE:
				return False
			self.title_ids
		except:
			return False
		return True

class MBoxInfo(GeneralClass):
	FILENAME = "MBoxInfo____"
	MAGIC = 0x6363
	@property
	def title_id(self):
		return '{:0>8X}'.format(struct.unpack('<I', self.data[4:8])[0])
	@property
	def hmac_key(self):
		return self.data[0x10:0x10 + 32]
	@property
	def ts_access(self):
		return self.data[0x34:0x34+12]
		#return struct.unpack('<q', self.data[0x34:0x34+12])[0]
	@property
	def ts_receive(self):
		return self.data[0x44:0x44+12]
		#return struct.unpack('<q', self.data[0x44:0x44+12])[0]

class BoxInfo(GeneralClass):
	FILENAME = "BoxInfo_____"
	MAGIC = 0x6262
	@property
	def size(self):
		return struct.unpack('<I', self.data[4:8])[0]
	@property
	def max_box_size(self):
		return struct.unpack('<I', self.data[8:0xC])[0]
	@property
	def current_box_size(self):
		return struct.unpack('<I', self.data[0xC:0xC+4])[0]
	@property
	def max_message_count(self):
		return struct.unpack('<I', self.data[0x10:0x14])[0]
	@property
	def current_message_count(self):
		return struct.unpack('<I', self.data[0x14:0x18])[0]
	@property
	def max_batch_size(self):
		return struct.unpack('<I', self.data[0x18:0x1C])[0]
	@property
	def max_message_size(self):
		return struct.unpack('<I', self.data[0x1C:0x1C+4])[0]
	@property
	def message_headers(self):
		ret = []
		for i in range(self.current_message_count):
			ret.append(RawMessage(self.data[0x1C+4 + i*0x70:0x1C+4 + (i+1)*0x70]))
		return ret
	@property
	def magic(self):
		return struct.unpack('<H', self.data[0:2])[0]
class RawMessage(GeneralClass):
	MAGIC = 0x6060
	@property
	def all_data(self):
		return len(self.data) > 0x70
	@property
	def size(self):
		return struct.unpack('<I', self.data[4:8])[0]
	@property
	def total_headers_size(self):
		return struct.unpack('<I', self.data[8:0xC])[0]
	@property
	def body_size(self):
		return struct.unpack('<I', self.data[0xC:0x10])[0]
	@property
	def title_id(self):
		return struct.unpack('<I', self.data[0x10:0x14])[0]
	@property
	def batch_id(self):
		return struct.unpack('<I', self.data[0x18:0x18+4])[0]
	@property
	def message_id(self):
		return struct.unpack('<q', self.data[0x20:0x20+8])[0]
	@message_id.setter
	def message_id(self, value):
		b = struct.pack('<q', value)
		for i in range(8):
			self.data[0x20 + i] = b[i]
	@property
	def message_id2(self):
		return struct.unpack('<q', self.data[0x2C:0x2C+8])[0]
	@message_id2.setter
	def message_id2(self, value):
		b = struct.pack('<q', value)
		for i in range(8):
			self.data[0x2C + i] = b[i]
	@property
	def send_method(self):
		"""
		0 - copy message_id (?) (street pass mii plaza, 00020800)
		1 - send_method 1->3 (?) (new super mario bros 2, 0007ad00; super mario maker, 001a0400)
		3 - no copy message_id (?) (a link between worlds, 000ec400)
		"""
		return struct.unpack('<B', self.data[0x35:0x36])[0]
	@property
	def is_unopen(self):
		return struct.unpack('<?', self.data[0x36:0x37])[0]
	@property
	def is_new(self):
		return struct.unpack('<?', self.data[0x37:0x38])[0]
	@property
	def sender_id(self):
		return struct.unpack('<Q', self.data[0x38:0x38+8])[0]
	@property
	def ts_sent(self):
		return self.data[0x48:0x48+12]
	@ts_sent.setter
	def ts_sent(self, value):
		for i in range(12):
			self.data[0x48 + i] = value[i]
	@property
	def ts_received(self):
		return self.data[0x54:0x54+12]
	@property
	def ts_created(self):
		return self.data[0x60:0x60+12]
	@property
	def send_count(self):
		return self.data[0x6C]
	@send_count.setter
	def send_count(self, value):
		self.data[0x6C] = value
	@property
	def forward_count(self):
		return self.data[0x6D]
	@forward_count.setter
	def forward_count(self, value):
		self.data[0x6D] = value
	@property
	def extra_headers(self):
		if not self.all_data:
			return []
		counter = 0x70
		ret = []
		while counter < self.full_headers_size:
			tmp = RawMessageExtraHeader(self.data[counter:counter+8])
			ret.append(RawMessageExtraHeader(self.data[counter:counter+tmp.size+8]))
			counter += tmp.size + 8
		return ret
	@property
	def body(self):
		if not self.all_data:
			return b''
		return self.data[self.full_headers_size:self.full_headers_size+self.body_size]
	def validate_header(self):
		try:
			return (self.magic == self.MAGIC
				and self.size == self.total_headers_size + self.body_size + 0x20
				and self.size < MAX_MESSAGE_SIZE)
		except:
			return False
	def validate(self):
		return self.validate_header() and self.size == len(self.data)

class RawMessageExtraHeader(GeneralClass):
	@property
	def type(self):
		return struct.unpack('<I', self.data[0:4])[0]
	@property
	def size(self):
		return struct.unpack('<I', self.data[4:8])[0]
	def get_data(self):
		return self.data[8:self.size+8]
