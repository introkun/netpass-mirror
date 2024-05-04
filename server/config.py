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

import yaml
from dotenv import dotenv_values

class Config:
	default_yaml = {
		"server": {
			"host": "localhost",
			"port": 8080,
		},
		"postgres": {
			"host": "localhost",
			"port": 5432,
		},
		"num_locations": 3,
		"version_env": "./version.env"
	}
	def __init__(self, filepath):
		with open(filepath, "r") as f:
			self.yaml = yaml.safe_load(f)
		self.default_yaml["num_locations"] = dotenv_values(self.get("version_env"))["NETPASS_NUM_LOCATIONS"]
	def get(self, param):
		this_yaml = self.yaml
		this_default_yaml = self.default_yaml
		for p in param.split("."):
			this_yaml = this_yaml[p] if this_yaml is not None and p in this_yaml else None
			this_default_yaml = this_default_yaml[p] if this_default_yaml is not None and p in this_default_yaml else None
		return this_yaml if this_yaml is not None else this_default_yaml
