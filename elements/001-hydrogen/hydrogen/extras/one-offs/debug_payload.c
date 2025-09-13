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
    
    const char *marker_pos = memmem(file_data, st.st_size, marker, strlen(marker));
    if (!marker_pos) {
        printf("❌ No marker found\n");
        munmap(file_data, st.st_size);
        return 1;
    }
    
    size_t marker_offset = (size_t)(marker_pos - (char*)file_data);
    printf("✅ Marker found at offset: %zu\n", marker_offset);
    
    // Check if we have enough bytes for the size
    if (marker_offset + strlen(marker) + 8 > (size_t)st.st_size) {
        printf("❌ Not enough bytes after marker for size field\n");
        munmap(file_data, st.st_size);
        return 1;
    }
    
    // Read the 8-byte size value
    const uint8_t *size_bytes = (uint8_t*)(marker_pos + strlen(marker));
    size_t payload_size = 0;
    for (int i = 0; i < 8; i++) {
        payload_size = (payload_size << 8) | size_bytes[i];
    }
    
    printf("Stored payload size: %zu bytes\n", payload_size);
    printf("Marker offset: %zu bytes\n", marker_offset);
    printf("Validation (payload_size <= marker_offset): %s\n", 
           payload_size <= marker_offset ? "✅ PASS" : "❌ FAIL");
    
    // Show the raw bytes of the size field
    printf("Size bytes (hex): ");
    for (int i = 0; i < 8; i++) {
        printf("%02x ", size_bytes[i]);
    }
    printf("\n");
    
    // Show last 50 bytes of file for debugging
    printf("\nLast 50 bytes of file:\n");
    const uint8_t *end_data = (uint8_t*)file_data + st.st_size - 50;
    for (int i = 0; i < 50; i++) {
        if (i % 16 == 0) printf("%04x: ", i);
        printf("%02x ", end_data[i]);
        if (i % 16 == 15) printf("\n");
    }
    printf("\n");
    
    munmap(file_data, st.st_size);
    return 0;
}
