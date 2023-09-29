#Don't include the interpreter here because we will have to invoke with:

# python3 pt-run.py array_index

#So we are getting a parameter passed to the python code; it is the array_index.

import subprocess, json


with open ("ds_good_ids.json","r") as rp:
    ds_list = json.load(rp)
    
for run in ds_list:
    subprocess.run(["./mdrtb.out","ds",str(run)]) 
