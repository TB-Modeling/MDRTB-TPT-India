#!/bin/bash -l

#SBATCH --partition=shared
#SBATCH --job-name=mdrtb-ds
#SBATCH --account=pkasaie1
#SBATCH --time=02:15:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-ds_%A_%a.out
#SBATCH --error=mdrtb-ds_%A_%a.err
#SBATCH --mail-type=end
#SBATCH --mail-user=jj.penn@gmail.com
#SBATCH --array=0-9999

module load gcc/6.4.0
module load boost

export LD_LIBRARY_PATH="/software/apps/boost/1.69.0/gcc/6.4/openmpi/3.1/lib:/software/apps/mpi/openmpi/3.1/gcc/6.4/lib:/software/apps/compilers/gcc/6.4.0/lib64:/software/apps/compilers/gcc/6.4.0/lib:/usr/lib:/usr/lib64:/software/centos7/lib64:/software/centos7/lib:/software/centos7/usr/lib64:/software/centos7/usr/lib:/software/apps/slurm/current/lib/slurm:/software/apps/slurm/current/lib"

# Execution
time ./mdrtb.out ds ${SLURM_ARRAY_TASK_ID}
