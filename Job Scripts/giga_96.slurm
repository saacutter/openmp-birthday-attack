#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=96
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=assignment1_giga_96
#SBATCH --output=3_giga_%j.out

make
./break_hash 3_giga_a.pdf 3_giga_b.pdf