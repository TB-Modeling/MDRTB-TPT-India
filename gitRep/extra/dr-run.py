#Don't include the interpreter here because we will have to invoke with:

# python3 dr-run.py array_index

#So we are getting a parameter passed to the python code; it is the array_index.

import sys, subprocess, json

array_index = int(sys.argv[1])

ds_id_index = array_index // 100
dr_id_index = array_index % 100

with open("good_ids.json", "r") as rp:
    ds_vals = json.loads(rp.read())

subprocess.run(["time","~/models/mdrtb/c.src/mdrtb.out","dr",str(ds_vals[ds_id_index]),str(dr_id_index)]) 
#print(f"./mdrtb.out dr {ds_vals[ds_id_index]} {dr_id_index}")
