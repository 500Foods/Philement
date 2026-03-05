import { defineConfig } from 'vite';
import { resolve } from 'path';

export default defineConfig(({ mode }) => {
  // Environment-driven paths — no hardcoded locations
  const lithiumRoot = process.env.LITHIUM_ROOT || resolve(__dirname);
  const deployDir = process.env.LITHIUM_DEPLOY;
  const isDeploy = mode === 'deploy' && deployDir;

  return {
    // Anchor Vite's root to $LITHIUM_ROOT so output paths are absolute
    root: lithiumRoot,

    // Base public path when served in production
    base: './',

    // Build options
    build: {
      // Output to $LITHIUM_DEPLOY when deploying, otherwise dist/ under root
      outDir: isDeploy ? resolve(deployDir) : resolve(lithiumRoot, 'dist'),

      // Don't empty the deploy dir (config may live there); do empty dist/
      emptyOutDir: !isDeploy,

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
          manualChunks(id) {
            if (id.includes('node_modules/tabulator-tables')) {
              return 'tabulator';
            }
            if (id.includes('node_modules/dompurify')) {
              return 'dompurify';
            }
            if (id.includes('node_modules/@codemirror')) {
              return 'codemirror';
            }
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
        '@': resolve(lithiumRoot, 'src')
      }
    },

    // CSS options
    css: {
      // Enable CSS source maps
      devSourcemap: true
    }
  };
});
