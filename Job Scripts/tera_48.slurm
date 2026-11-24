#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=48
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=assignment1_tera_48
#SBATCH --output=4_tera_%j.out

make
./break_hash 4_tera_a.pdf 4_tera_b.pdf