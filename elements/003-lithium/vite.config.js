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

      // Build options - Optimized for production
      build: {
        // Output to $LITHIUM_DEPLOY when deploying, otherwise dist/ under root
        outDir: isDeploy ? resolve(deployDir) : resolve(lithiumRoot, 'dist'),

        // Don't empty the deploy dir (config may live there); do empty dist/
        emptyOutDir: !isDeploy,

        // Generate source maps for debugging
        sourcemap: true,

        // Enable all optimizations for production
        minify: 'terser',
        cssMinify: true,
        treeshake: true,

        // Target modern browsers
        target: 'esnext',

        // Code splitting - let Rollup create optimal chunks
        rollupOptions: {
          output: {
            manualChunks: {
              // Vendor libraries that change less frequently
              'vendor-ui': ['tabulator-tables', 'json-tree.js'],
              'vendor-editor': ['@codemirror/lang-markdown', '@codemirror/lang-css', '@codemirror/lang-html', 
                '@codemirror/lang-javascript', '@codemirror/lang-json', '@codemirror/lang-sql',
                '@codemirror/state', '@codemirror/theme-one-dark', '@codemirror/view', '@codemirror/commands'],
              'vendor-utils': ['marked', 'dompurify', 'sql-formatter', 'country-flag-icons']
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
