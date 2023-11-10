# -*- coding: utf-8 -*-
import json

functionManager = currentProgram.getFunctionManager()
baseAddress = currentProgram.getImageBase()
USER_DEFINED = ghidra.program.model.symbol.SourceType.USER_DEFINED

def get_addr(addr):
	return baseAddress.add(addr)

def set_name(addr, name):
	name = name.replace(' ', '-')
	createLabel(addr, name, True, USER_DEFINED)

def make_function(start):
	func = getFunctionAt(start)
	if func is None:
		createFunction(start, None)

f = askFile("script.json from UE4Dumper", "Open")
json_data = json.loads(open(f.absolutePath, 'rb').read().decode('utf-8'))

monitor.initialize(len(json_data['Functions']))
monitor.setMessage("Methods")
for func_entry in json_data['Functions']:
	addr = get_addr(func_entry["Address"])
	name = func_entry["Name"].encode("utf-8")
	set_name(addr, name)
	monitor.incrementProgress(1)

print('Script finished!')
