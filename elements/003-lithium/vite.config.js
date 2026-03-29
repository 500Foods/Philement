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

        // Chunk size warning threshold (largest vendor chunk is ~581 kB)
        chunkSizeWarningLimit: 708,

      // Code splitting - chunk by library purpose (each manager loads only what it needs)
      rollupOptions: {
        output: {
          manualChunks: {
            // Heavy UI libraries - split by purpose for cleaner manager dependency graphs
            'vendor-tabulator': ['tabulator-tables'],              // Data grids (queries, style-manager, login)
            'vendor-codemirror': [                                  // Code/text editors (queries, login, style-manager, session-log)
              '@codemirror/lang-markdown', '@codemirror/lang-css', '@codemirror/lang-html',
              '@codemirror/lang-javascript', '@codemirror/lang-json', '@codemirror/lang-sql',
              '@codemirror/state', '@codemirror/theme-one-dark', '@codemirror/view', '@codemirror/commands'
            ],
            'vendor-editor': ['suneditor'],                        // Rich text editor (lookups only)

            // Lighter utilities grouped together (change less frequently)
            'vendor-utils': ['marked', 'dompurify', 'sql-formatter', 'country-flag-icons']
          }
        },
        // Suppress warnings from node_modules (external dependencies we cannot control)
        onwarn(warning, warn) {
          if (warning.id && warning.id.includes('node_modules')) {
            return; // Skip warnings from node_modules
          }
          warn(warning);
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
