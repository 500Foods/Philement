#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <stdint.h>

bool check_payload_exists(const char *executable_path, const char *marker, size_t *size) {
    if (!executable_path || !marker || !size) return false;
    
    int fd = open(executable_path, O_RDONLY);
    if (fd == -1) {
        printf("Failed to open executable: %s\n", executable_path);
        return false;
    }
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        printf("Failed to stat executable\n");
        return false;
    }
    
    void *file_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (file_data == MAP_FAILED) {
        printf("Failed to mmap executable\n");
        return false;
    }
    
    bool result = false;
    const char *marker_pos = memmem(file_data, st.st_size, marker, strlen(marker));
    
    if (marker_pos && st.st_size > 0) {
        size_t file_size = (size_t)st.st_size;
        size_t marker_offset = (size_t)(marker_pos - (char*)file_data);
        printf("Found marker at offset: %zu (0x%zx)\n", marker_offset, marker_offset);
        printf("File size: %zu\n", file_size);
        
        if (marker_offset + strlen(marker) + 8 <= file_size) {
            const uint8_t *size_bytes = (uint8_t*)(marker_pos + strlen(marker));
            *size = 0;
            for (int i = 0; i < 8; i++) {
                *size = (*size << 8) | size_bytes[i];
            }
            
            printf("Payload size from file: %zu\n", *size);
            printf("Marker offset: %zu\n", marker_offset);
            printf("Validation: size <= marker_offset? %s\n", (*size <= marker_offset) ? "YES" : "NO");
            
            if (*size > 0 && *size <= 100 * 1024 * 1024 && *size <= marker_offset) {
                result = true;
                printf("✅ Payload validation PASSED\n");
            } else {
                printf("❌ Payload validation FAILED\n");
                if (*size == 0) printf("  - Size is zero\n");
                if (*size > 100 * 1024 * 1024) printf("  - Size too large\n");
                if (*size > marker_offset) printf("  - Size larger than marker offset\n");
            }
        } else {
            printf("❌ Not enough bytes after marker for size field\n");
        }
    } else {
        printf("❌ Marker not found\n");
    }
    
    munmap(file_data, st.st_size);
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <executable_path>\n", argv[0]);
        return 1;
    }
    
    const char *marker = "<<< HERE BE ME TREASURE >>>";
    size_t payload_size = 0;
    
    printf("Testing payload detection for: %s\n", argv[1]);
    printf("Looking for marker: %s\n", marker);
    printf("----------------------------------------\n");
    
    bool found = check_payload_exists(argv[1], marker, &payload_size);
    
    printf("----------------------------------------\n");
    printf("Result: %s\n", found ? "✅ PAYLOAD DETECTED" : "❌ NO PAYLOAD FOUND");
    
    return found ? 0 : 1;
}
