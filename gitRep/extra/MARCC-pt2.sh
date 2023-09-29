#!/bin/bash -l

#Running the PT scenario after creating the DR populations

#SBATCH --partition=shared
#SBATCH --account=pkasaie1
#SBATCH --job-name=mdrtb-pt2
#SBATCH --time=1:30:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-pt2-%A-%a.out
#SBATCH --error=mdrtb-pt2-%A-%a.err
#SBATCH --array=0-39999
#SBATCH --mail-type=end
#SBATCH --mail-user=jj.penn@gmail.com

module load python/3.8
module load gcc/6.4.0
module load boost

base=40000

python3 pt-run.py $((SLURM_ARRAY_TASK_ID+base))
