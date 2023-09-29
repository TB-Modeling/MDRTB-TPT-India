#Don't include the interpreter here because we will have to invoke with:

# python3 pt-run.py array_index

#So we are getting a parameter passed to the python code; it is the array_index.

import sys, subprocess, json

array_index = int(sys.argv[1])

reps = 25
reptype = 4 #Placebo, two drugs and baseline
runs_per_id = reps * reptype

dsdr_id_index = array_index // runs_per_id
pt_scen_index = str((array_index % runs_per_id) // reps)
rep_index = str((array_index % runs_per_id) % reps)
pt_index = str(dsdr_id_index)

with open("mdrtb_9_1000_pt.json", "r") as rp:
    ds_dr_vals = json.load(rp)[dsdr_id_index]

subprocess.run(["./mdrtb.out","pt",str(ds_dr_vals[0]),str(ds_dr_vals[1]),pt_scen_index,rep_index,pt_index]) 
#print(f"./mdrtb.out pt {ds_dr_vals[0]} {ds_dr_vals[1]} {pt_scen_index} {rep_index} {pt_index}")
