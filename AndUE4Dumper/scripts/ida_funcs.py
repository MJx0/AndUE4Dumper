# -*- coding: utf-8 -*-
import json

path = idaapi.ask_file(False, '*.json', 'script.json')
json_data = json.loads(open(path, 'rb').read().decode('utf-8'))

imageBase = idaapi.get_imagebase()

for func_entry in json_data['Functions']:
    name = func_entry['Name']
    addr = imageBase+func_entry['Address']
    ret = idc.set_name(addr, name, SN_NOWARN | SN_NOCHECK)
    if ret == 0:
       new_name = name+'_'+str(addr)
       ret = idc.set_name(addr, new_name, SN_NOWARN | SN_NOCHECK)