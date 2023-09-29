#Generate the dr populations for the PT runs

#Don't include the interpreter here because we will have to invoke with:

# python3 pop-gen.py array_index

#So we are getting a parameter passed to the python code; it is the array_index.

import sys, subprocess, json

array_index = int(sys.argv[1])

with open("mdrtb_9_76_pops.json", "r") as rp:
    ds_dr_vals = json.load(rp)[array_index]

subprocess.run(["./mdrtb.out","dr",str(ds_dr_vals[0]),str(ds_dr_vals[1])]) 
#print(f"./mdrtb.out dr {ds_dr_vals[0]} {ds_dr_vals[1]}")
