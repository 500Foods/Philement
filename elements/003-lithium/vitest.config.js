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
        'src/core/*.js',
        'src/shared/**/*.js',
        'src/managers/**/*.js',
      ],
      exclude: [
        'node_modules/',
        'tests/',
        'dist/',
        '**/*.config.js',
        'src/init/**',
      ],
    },
  },
});
