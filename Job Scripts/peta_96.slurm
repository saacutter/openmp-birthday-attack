#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=96
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=assignment1_peta_96
#SBATCH --output=5_peta_%j.out

make
./break_hash 5_peta_a.pdf 5_peta_b.pdf