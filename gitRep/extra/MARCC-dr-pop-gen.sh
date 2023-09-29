#!/bin/bash -l

#Generating the DR populations for the PT runs

#SBATCH --partition=shared
#SBATCH --job-name=mdrtb-pop-gen
#SBATCH --account=pkasaie1
#SBATCH --time=3:00:00
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --nodes=1
#SBATCH --output=mdrtb-pop-gen-%A-%a.out
#SBATCH --error=mdrtb-pop-gen-%A-%a.err
#SBATCH --array=0-75
#SBATCH --mail-type=end
#SBATCH --mail-user=jj.penn@gmail.com

module load python/3.8
module load gcc/6.4.0
module load boost

python3 pop-gen.py $SLURM_ARRAY_TASK_ID
