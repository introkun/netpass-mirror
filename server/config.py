import yaml

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
		"num_locations": 3
	}
	def __init__(self, filepath):
		with open(filepath, "r") as f:
			self.yaml = yaml.safe_load(f)
	def get(self, param):
		this_yaml = self.yaml
		this_default_yaml = self.default_yaml
		for p in param.split("."):
			this_yaml = this_yaml[p] if this_yaml is not None and p in this_yaml else None
			this_default_yaml = this_default_yaml[p] if this_default_yaml is not None and p in this_default_yaml else None
		return this_yaml if this_yaml is not None else this_default_yaml
