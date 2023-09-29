#!/bin/bash -l

#Generating the HH demographic info 

#SBATCH --partition=shared
#SBATCH --job-name=mdrtb-hhdemog
#SBATCH --account=pkasaie1
#SBATCH --time=1:00:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-hhdemog-%A-%a.out
#SBATCH --error=mdrtb-hhdemog-%A-%a.err
#SBATCH --array=0-56
#SBATCH --mail-type=end
#SBATCH --mail-user=jj.penn@gmail.com

module load python/3.8
module load gcc/6.4.0
module load boost

python3 hh-demog.py $SLURM_ARRAY_TASK_ID
