import { defineConfig } from 'vite';

export default defineConfig({
  // Base public path when served in production
  base: './',

  // Build options
  build: {
    // Output directory
    outDir: 'dist',

    // Generate source maps for debugging
    sourcemap: true,

    // Minify for production
    minify: 'terser',

    // Target modern browsers
    target: 'esnext',


    // Rollup options
    rollupOptions: {
      output: {
        // Manual chunks for better caching
        manualChunks: {
          vendor: ['idb']
        }
      }
    }
  },

  // Development server options
  server: {
    // Port for dev server
    port: 3000,

    // Open browser automatically
    open: true,

    // Enable HTTPS for PWA development
    https: false, // Set to true if you have SSL certificates
  },

  // Path resolution
  resolve: {
    alias: {
      '@': '/src'
    }
  },

  // CSS options
  css: {
    // Enable CSS source maps
    devSourcemap: true
  }
});