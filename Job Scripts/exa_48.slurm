#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=48
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=assignment1_exa_48
#SBATCH --output=6_exa_%j.out

make
./break_hash 6_exa_a.pdf 6_exa_b.pdf