# OpenMP Birthday Attack
This program attempts to find a hash collision between any two provided files through a birthday attack. That is, it attempts to modify the input files such that they produce the same hash. The provided files must be a PDF with the following header structure:

| Byte Offset | Contents     |
|-------------|--------------|
| 0-7         | %PDF-1.4     |
| 8           | \n           |
| 9-15        | %NONCE=      |
| 16-31       | nonce        |
| 32          | \n           |
| 33-44       | %STUDENT_ID= |
| 45-52       | student ID   |
| 53          | \n           |

Due to the structure of these files, the only bytes that can be adjusted are bytes 16-31 (the nonce) and 45-52 (the student ID). However, as the student ID is fixed for all files (24314307), only the nonce can be adjusted. The program, therefore, attempts to find a hash collision by changing the nonce of each file.

Note that, once a collision is found and verified, the original files are overwritten with the data that was found to produce the collision. As only the nonce and student ID comments in the header are changed, the files will look identical with only these comments changing so this should be kept in mind when using this program.

## Running the Program
This program is compiled using `make`, which compiles the parallel implementation by default. It accepts up to three command-line arguments; a file path (file A), another file path (file B), and an optional number of threads to use (by default the maximum number of threads that can be used will be used). The program can be invoked in the following way:
```
./break_hash FILE_A FILE_B [NUM_THREADS]
```

It should be noted that using a different number of threads can result in different hashes being found, particularly when the number of possible collisions between file pairs increases. This is because OpenMP's loop scheduling changes where each thread starts and ends as the number of threads change, which can cause the first collision found (and hence the collision returned) to be different. This is not expected to be an issue as the collision is still verified, but it should be kept in mind if the detected collision is different between runs.

The serial implementation can be compiled using `make serial`. This creates the `break_hash_serial` binary which only takes two command-line arguments (the two file paths). It does not accept a number of threads to use as, by definition of being serial, only has access to one thread. As such, it is invoked in the following way:
```
./break_hash_serial FILE_A FILE_B
```

Any binary that has been compiled can be removed from the system using `make clean`. This includes the `toy_hash.o` object file, `break_hash` parallel implementation binary, and `break_hash_serial` serial implementation binary.

Note that this program requires, at minimum, a few gigabytes of memory. It will require at least 2GiB of memory for the hash table, and some extra memory for the file pair. As each thread creates its own local copy of the files in addition to the original copies of each file, this will require `(NUM_THREADS + 1) * (SIZE_OF_FILE)` memory for each file (and hence approximately doubled for each file pair). The largest of the provided files is the `6_exa` file pair which are each 921,600 bytes, or approximately 0.879MiB (or 1.758MiB in total). Consequently, assuming that there are 96 cores being used for a particular run, an additional 170.53MiB of memory will be required to execute the program. This will grow with the size of the files, but 5-10GiB of memory should be enough in most instances.

If this program needs to be run on Kaya (or any other HPC system), a SLURM script can be used. For reference purposes, the following script can be modified to suit the job it is required for:
```sh
#!/bin/bash

#SBATCH --nodes=1
#SBATCH --cpus-per-task=96
#SBATCH --time=00:15:00
#SBATCH --mem=12G
#SBATCH --partition=cits3402
#SBATCH --job-name=kilo_96
#SBATCH --output=kilo_96.out
#SBATCH --mail-type=ALL
#SBATCH --mail-user=<email>

make
./break_hash 1_kilo_a.pdf 1_kilo_b.pdf
```