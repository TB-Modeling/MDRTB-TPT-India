#!/usr/bin/python3

"""
After the DS run has completed, run this script which will check on the successful DS runs
and fork off an array of DR runs for each
"""

import glob, os, stat, subprocess, json

script_text = """#!/bin/bash -l

#Using the MARCC recommended settings

#SBATCH --partition=shared
#SBATCH --job-name=mdrtb-dr-{0}
#SBATCH --time=6:00:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-dr-{0}_job_%A_%a.out
#SBATCH --error=mdrtb-dr-{0}_job_%A_%a.err
#SBATCH --array=0-49

module add gcc/6.4.0
module add boost

export LD_LIBRARY_PATH="/software/apps/boost/1.69.0/gcc/6.4/openmpi/3.1/lib:/software/apps/mpi/openmpi/3.1/gcc/6.4/lib:/software/apps/compilers/gcc/6.4.0/lib64:/software/apps/compilers/gcc/6.4.0/lib:/usr/lib:/usr/lib64:/software/centos7/lib64:/software/centos7/lib:/software/centos7/usr/lib64:/software/centos7/usr/lib:/software/apps/slurm/current/lib/slurm:/software/apps/slurm/current/lib"

./mdrtb.out dr {0} $SLURM_ARRAY_TASK_ID
"""

#This file doesn't exist; have to make it.  It's a list of all the
#successful DS run ids

with open("run9_drids.json","r") as fp:
    ds_id_list = json.load(fp)

#ds_id_list = [int(x.split("_")[3]) for x in glob.glob("*.gz")]

#ds_id_first = ds_id_list[:1250]
ds_id_second = ds_id_list[1250:]

for (idx, ds_id) in enumerate(ds_id_second):
    filename = f"slurm-dr-run-{idx}.sh"
    with open (filename, "w") as wp:
        wp.write( script_text.format(ds_id) )
    subprocess.run(["sbatch",filename])
