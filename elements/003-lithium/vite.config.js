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

        // Disable source maps for production builds (significant performance boost)
        sourcemap: false,

        // Disable gzip-compressed size reporting for faster builds
        reportCompressedSize: false,

        // Enable all optimizations for production (use fast Oxc minifier)
        minify: 'oxc',
        cssMinify: true,
        treeshake: true,

        // Target modern browsers
        target: 'esnext',

        // Chunk size warning threshold (allow larger chunks for heavy libs)
        chunkSizeWarningLimit: 1500,

// Improved code splitting for better performance
      rolldownOptions: {
        output: {
          manualChunks(id) {
            // Handle node_modules libraries
            if (id.includes('node_modules')) {
              // Keep heavy libraries in separate chunks
              if (id.includes('tabulator')) return 'vendor-tabulator';
              if (id.includes('codemirror')) return 'vendor-codemirror';
              if (id.includes('luxon')) return 'vendor-luxon';
              if (id.includes('marked')) return 'vendor-marked';
              if (id.includes('shepherd')) return 'vendor-shepherd';
              if (id.includes('suneditor')) return 'vendor-suneditor';

              // Group smaller utilities together
              return 'vendor-utils';
            }

            // Group all manager components together (reduces file count)
            if (id.includes('/managers/') || id.includes('manager')) {
              return 'managers';
            }

            // Everything else stays in the main bundle
          }
        },
        // Suppress warnings from node_modules (external dependencies we cannot control)
        onwarn(warning, warn) {
          if (warning.id && warning.id.includes('node_modules')) {
            return; // Skip warnings from node_modules
          }
          // Treat CSS syntax errors as fatal build failures
          if (warning.code === 'CSS_SYNTAX_ERROR') {
            throw new Error(`CSS Syntax Error: ${warning.message}`);
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
