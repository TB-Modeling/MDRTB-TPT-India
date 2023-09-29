#!/bin/bash -l

#SBATCH --partition=shared
#SBATCH --job-name=mdrtb-bridge
#SBATCH --time=00:30:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-bridge%A_%a.out
#SBATCH --error=mdrtb-bridge%A_%a.err

module load python/3.8

export LD_LIBRARY_PATH="/software/apps/boost/1.69.0/gcc/6.4/openmpi/3.1/lib:/software/apps/mpi/openmpi/3.1/gcc/6.4/lib:/software/apps/compilers/gcc/6.4.0/lib64:/software/apps/compilers/gcc/6.4.0/lib:/usr/lib:/usr/lib64:/software/centos7/lib64:/software/centos7/lib:/software/centos7/usr/lib64:/software/centos7/usr/lib:/software/apps/slurm/current/lib/slurm:/software/apps/slurm/current/lib"

# Execution
python3 ds-stage-over.py
