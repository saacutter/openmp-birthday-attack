#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=48
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=assignment1_mega_48
#SBATCH --output=2_mega_%j.out

make
./break_hash 2_mega_a.pdf 2_mega_b.pdf