import { defineConfig } from 'vite';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const hotFile = path.resolve(__dirname, 'public_html/hot');
const devServerUrl = 'http://localhost:5173';

/**
 * Writes a "hot" file when `npm run dev` is running so the PHP Vite
 * helper knows to point at the dev server instead of built assets.
 * Removed automatically when the dev server stops.
 */
function keelHotFilePlugin() {
  return {
    name: 'keel-hot-file',
    configureServer(server) {
      fs.writeFileSync(hotFile, devServerUrl);
      server.httpServer?.once('close', () => {
        if (fs.existsSync(hotFile)) fs.unlinkSync(hotFile);
      });
      process.on('exit', () => {
        if (fs.existsSync(hotFile)) fs.unlinkSync(hotFile);
      });
    },
  };
}

export default defineConfig({
  plugins: [keelHotFilePlugin()],
  server: {
    port: 5173,
    strictPort: true,
    cors: true,
  },
  build: {
    outDir: 'public_html/assets',
    manifest: true,
    emptyOutDir: true,
    rollupOptions: {
      input: {
        app: 'resources/js/app.js',
      },
    },
  },
});
