#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <omp.h>
#include "toy_hash.h"

#define NONCE_OFFSET 16
#define NONCE_LENGTH 16
#define STUDENT_ID_OFFSET 45
#define STUDENT_ID_LENGTH 8
#define TABLE_CAPACITY (1u << 27)
#define MAX_A_TRIALS ((uint64_t)1 << 24)
#define MAX_B_TRIALS ((uint64_t)1 << 27)

typedef struct {
    unsigned char *contents;
    size_t size;
    const char *path;
} file;

typedef struct {
    _Atomic uint64_t hash;
    _Atomic uint64_t nonce_plus;
} TableEntry;

// These are globally defined so every function can access them as required
static TableEntry *table;
static file *f1;
static file *f2;

static void table_insert(uint64_t hash, uint64_t nonce);
static int table_lookup(uint64_t hash, uint64_t *match_nonce);
static void set_nonce(unsigned char *pdf, uint64_t nonce);
static void load_file(const char *path, file *data);
static void write_file(file *pdf);
static void cleanup();

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s FILE_1 FILE_2 [NUM_THREADS]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create the file objects
    f1 = malloc(sizeof(file));
    f2 = malloc(sizeof(file));

    // Read the contents of each file
    load_file(argv[1], f1);
    load_file(argv[2], f2);

    // Initialise the table to check the file hashes
    table = calloc(TABLE_CAPACITY, sizeof(TableEntry)); 
    if (!table) {
        fprintf(stderr, "Not enough memory could be allocated\n");
        exit(EXIT_FAILURE);
    }

    // Set the number of threads (if provided)
    if (argc == 4) {
        int num_threads = atoi(argv[3]); 
        if (num_threads > 0) omp_set_num_threads(num_threads);
    }

    uint64_t found_n1 = 0;
    uint64_t found_n2 = 0;
    int collision_found = 0;
    double t0 = omp_get_wtime();
    #pragma omp parallel
    {
        // Create a local copy of the first file for each thread
        unsigned char *localA = malloc(f1->size);
        if (!localA) {
            fprintf(stderr, "Building A failed\n");
            exit(1);
        }
        memcpy(localA, f1->contents, f1->size);

        // Calculate the first 2^24 hashes for the first file and store them in memory
        #pragma omp for schedule(static)
        for (uint64_t nonce = 0; nonce < MAX_A_TRIALS; nonce++) {
            set_nonce(localA, nonce);
            uint64_t hash = toy_hash(localA, f1->size);
            table_insert(hash, nonce);
        }

        // Create a local copy of the second file for each thread
        unsigned char *localB = malloc(f2->size);
        if (!localB) {
            fprintf(stderr, "Building B failed\n");
            exit(1);
        }
        memcpy(localB, f2->contents, f2->size);

        // Calculate the first 2^27 hashes for the second file and see if they can be found in the table
        #pragma omp for schedule(static)
        for (uint64_t nonce = 0; nonce < MAX_B_TRIALS; nonce++) {
            if (collision_found) continue; // If a collision has already been found, don't continue unnecessarily processing

            // Calculate the hash for the current nonce candidate
            set_nonce(localB, nonce);
            uint64_t hB = toy_hash(localB, f2->size);

            // See if the hash already exists in the table
            uint64_t a_nonce;
            int hit = table_lookup(hB, &a_nonce);

            // If a potential collision was found, verify it before anything happens
            if (hit) {
                printf("Potential collision found with a_nonce=%lx and b_nonce=%lx. Verifying...\n", a_nonce, nonce);
                set_nonce(localA, a_nonce);
                if (toy_hash(localB, f2->size) == toy_hash(localA, f1->size)) {
                    printf("Collision verified with hash=%lx.\n", hB);
                    #pragma omp critical(result)
                    {
                        if (!collision_found) {
                            collision_found = 1;
                            found_n1 = a_nonce;
                            found_n2 = nonce;
                        }
                    }
                } else {
                    printf("The collision was not verified.\n");
                }
            }
        }  
    }
    double t1 = omp_get_wtime();

    // Print the time taken to execute the parallel region
    printf("Time=%.3fs\n", t1 - t0);

    // Return if no collision was found
    if (!collision_found) {
        fprintf(stderr, "No collisions found\n");
        cleanup();
        return 1;
    }

    // Set the nonce of each file to be what was found
    set_nonce(f1->contents, found_n1);
    set_nonce(f2->contents, found_n2);

    // Write the computed nonces to the file
    write_file(f1);
    write_file(f2);

    cleanup();
}

static void table_insert(uint64_t hash, uint64_t nonce) {
    // Identify a candidate slot in the table for the hash
    uint32_t slot = (uint32_t)(hash % TABLE_CAPACITY);
    uint32_t start = slot;

    // Try to insert the hash at any available slot (and return if all slots are full)
    do {
        uint64_t expected = 0;
        if (atomic_compare_exchange_strong(&table[slot].hash, &expected, hash)) {
            atomic_store_explicit(&table[slot].nonce_plus, nonce + 1, memory_order_release);
            return;
        }

        // If the expected slot is the hash, then don't store a duplicate
        if (expected == hash) {
            return;
        }

        // If the slot was filled, try the next slot until an empty one is found
        slot = (slot + 1) % TABLE_CAPACITY;
    } while (slot != start);

    return;
}

static int table_lookup(uint64_t hash, uint64_t *match_nonce) {
    // Calculate the slot where the hash should be if it is in the table (based on how they are inserted)
    uint32_t slot = (uint32_t)(hash % TABLE_CAPACITY);
    uint32_t start = slot;

    // Search all potential slots to see if the hash is in the table (and return if it could not be found)
    do {
        uint64_t h = atomic_load_explicit(&table[slot].hash, memory_order_acquire);

        // If the table slot's hash is 0 then the hash is not in the table 
        if (h == 0) return 0;

        // If the current slot holds the hash, a collision has been found so it should be returned
        if (h == hash) {
            uint64_t np;
            do {
                np = atomic_load_explicit(&table[slot].nonce_plus, memory_order_acquire);
            } while (np == 0);
            *match_nonce = np - 1;
            return 1;
        }

        // If the hash did not match, try the next one
        slot = (slot + 1) % TABLE_CAPACITY;
    } while (slot != start);

    return 0;
}

static void set_nonce(unsigned char *pdf, uint64_t nonce) {
    static const char hex[] = "0123456789abcdef";

    for (int i = NONCE_LENGTH - 1; i >= 0; --i) {
        pdf[NONCE_OFFSET + i] = (unsigned char)hex[nonce & 0xf];
        nonce >>= 4;
    }
}

static void set_student_number(unsigned char *pdf, const char *id) {
    memcpy(pdf + STUDENT_ID_OFFSET, id, STUDENT_ID_LENGTH);
}

static void load_file(const char *path, file *data) {
    // Open the file for reading
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", path);
        exit(EXIT_FAILURE);
    }

    // Determine the size of the file (makes it easier to allocate the file in one chunk in memory rather than reading it sequentially
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "Could not seek %s\n", path);
        exit(EXIT_FAILURE);
    }
    size_t size = (size_t)ftell(f);
    rewind(f);

    // Allocate enough memory to store the file's contents and read its contents
    unsigned char *contents = (unsigned char *)malloc(size);
    if (!contents || fread(contents, sizeof(unsigned char), size, f) != size) {
        fprintf(stderr, "Failed to read %s\n", path);
        exit(EXIT_FAILURE);
    }
    fclose(f);

    // Set the student number of the file
    set_student_number(contents, "24314307");

    // Set the fields of the file object
    data->size = size;
    data->contents = contents;
    data->path = path;
}

static void write_file(file *pdf) {
    // Open the file for reading
    FILE *file = fopen(pdf->path, "wb");
    if (!file) {
        fprintf(stderr, "Could not open %s\n", pdf->path);
        exit(EXIT_FAILURE);
    }

    // Write the modified PDF
    if (!fwrite(pdf->contents, sizeof(unsigned char), pdf->size, file)) {
        fprintf(stderr, "Failed to write %s\n", pdf->path);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

static void cleanup() {
    free(table);
    free(f1);
    free(f2);
}