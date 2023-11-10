# -*- coding: utf-8 -*-
import json

imageBase = idaapi.get_imagebase()

def get_addr(addr):
	return imageBase + addr

def set_name(addr, name):
	ret = idc.set_name(addr, name, SN_NOWARN | SN_NOCHECK)
	if ret == 0:
		new_name = name + '_' + str(addr)
		ret = idc.set_name(addr, new_name, SN_NOWARN | SN_NOCHECK)

def make_function(start, end):
	next_func = idc.get_next_func(start)
	if next_func < end:
		end = next_func
	if idc.get_func_attr(start, FUNCATTR_START) == start:
		ida_funcs.del_func(start)
	ida_funcs.add_func(start, end)

path = idaapi.ask_file(False, '*.json', 'script.json')
json_data = json.loads(open(path, 'rb').read().decode('utf-8'))

for func_entry in json_data['Functions']:
    addr = get_addr(func_entry["Address"])
    name = func_entry["Name"].encode("utf-8")
    set_name(addr, name)

print('Script finished!')