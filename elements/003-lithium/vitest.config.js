import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    environment: 'happy-dom',
    globals: true,
    setupFiles: ['./tests/setup.js'],
    include: ['tests/unit/**/*.test.js', 'tests/integration/**/*.test.js'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      reportsDirectory: './coverage',
      include: [
        'src/core/event-bus.js',
        'src/core/jwt.js',
        'src/core/json-request.js',
        'src/core/permissions.js',
        'src/core/config.js',
        'src/core/utils.js',
        'src/shared/**/*.js',
        'src/managers/**/*.js',
      ],
      exclude: [
        'node_modules/',
        'tests/',
        'dist/',
        '**/*.config.js',
        // Legacy code kept for reference but not part of new architecture
        'src/modules/**',
        'src/init/**',
        'src/core/logger/**',
        'src/core/network/**',
        'src/core/router/**',
        'src/core/storage/**',
      ],
    },
  },
});
