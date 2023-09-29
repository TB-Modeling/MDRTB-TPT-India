#Don't include the interpreter here because we will have to invoke with:

# python3 pt-run.py array_index

#So we are getting a parameter passed to the python code; it is the array_index.

import sys, subprocess, json

array_index = int(sys.argv[1])

with open ("full_ds_id_list.json","r") as rp:
    ds_val = json.load(rp)[array_index]
    
subprocess.run(["./mdrtb.out","ds",str(ds_val)]) 
