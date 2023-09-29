#Don't include the interpreter here because we will have to invoke with:

# python3 dr-run.py array_index

#So we are getting a parameter passed to the python code; it is the array_index.

import sys, subprocess, json

array_index = int(sys.argv[1])

with open("128_missing_ds.json", "r") as rp:
    ds = json.loads(rp.read())[array_index]

subprocess.run(["./mdrtb.out","ds",str(ds)]) 
#print(f"./mdrtb.out dr {ds_vals[ds_id_index]} {dr_id_index}")
