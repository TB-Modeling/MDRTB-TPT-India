#!/bin/bash -l

#Running the PT scenario after creating the DR populations

#SBATCH --partition=shared
#SBATCH --account=pkasaie1
#SBATCH --job-name=mdrtb-pt-m
#SBATCH --time=1:30:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-ptm-%A-%a.out
#SBATCH --error=mdrtb-ptm-%A-%a.err
#SBATCH --array=0-1
#SBATCH --mail-type=end
#SBATCH --mail-user=jj.penn@gmail.com

module load python/3.8
module load gcc/6.4.0
module load boost

python3 pt-run-missing.py $SLURM_ARRAY_TASK_ID
