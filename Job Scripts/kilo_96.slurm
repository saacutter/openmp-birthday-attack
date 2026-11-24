#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=96
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=assignment1_kilo_96
#SBATCH --output=1_kilo_%j.out

make
./break_hash 1_kilo_a.pdf 1_kilo_b.pdf