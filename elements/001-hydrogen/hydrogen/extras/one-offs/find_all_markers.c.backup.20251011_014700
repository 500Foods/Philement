#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdint.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <binary_file>\n", argv[0]);
        return 1;
    }
    
    const char *marker = "<<< HERE BE ME TREASURE >>>";
    const char *filename = argv[1];
    
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }
    
    void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (file_data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    printf("File: %s\n", filename);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Searching for all occurrences of marker...\n\n");
    
    char *search_start = (char*)file_data;
    size_t remaining_size = st.st_size;
    int occurrence = 1;
    
    while (remaining_size >= strlen(marker)) {
        char *marker_pos = memmem(search_start, remaining_size, marker, strlen(marker));
        if (!marker_pos) {
            break;
        }
        
        size_t marker_offset = (size_t)(marker_pos - (char*)file_data);
        printf("Occurrence #%d:\n", occurrence);
        printf("  Offset: %zu\n", marker_offset);
        
        // Check if we have enough bytes for the size field
        if (marker_offset + strlen(marker) + 8 <= (size_t)st.st_size) {
            const uint8_t *size_bytes = (uint8_t*)(marker_pos + strlen(marker));
            size_t payload_size = 0;
            for (int i = 0; i < 8; i++) {
                payload_size = (payload_size << 8) | size_bytes[i];
            }
            
            printf("  Size bytes (hex): ");
            for (int i = 0; i < 8; i++) {
                printf("%02x ", size_bytes[i]);
            }
            printf("\n");
            printf("  Stored payload size: %zu bytes\n", payload_size);
            printf("  Validation (payload_size <= marker_offset): %s\n", 
                   payload_size <= marker_offset ? "✅ PASS" : "❌ FAIL");
        } else {
            printf("  ❌ Not enough bytes after marker for size field\n");
        }
        
        printf("\n");
        
        // Move search position past this marker
        search_start = marker_pos + strlen(marker);
        remaining_size = st.st_size - (search_start - (char*)file_data);
        occurrence++;
    }
    
    if (occurrence == 1) {
        printf("No marker found in file.\n");
    }
    
    munmap(file_data, st.st_size);
    return 0;
}
