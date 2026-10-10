import { spawnSync } from 'node:child_process';
import { brotliCompressSync, brotliDecompressSync, gzipSync, gunzipSync, constants as zc } from 'node:zlib';
import { createHash } from 'node:crypto';
import { readFileSync, writeFileSync, mkdirSync } from 'node:fs';
import { resolve, dirname, join } from 'node:path';

const root = resolve(import.meta.dirname, '..');
const env = process.argv[2] || 'esp32dev';
const outDir = resolve(root, 'release-assets');
const firmware = resolve(root, '.pio', 'build', env, 'firmware.bin');
const packagePath = resolve(outDir, `firmware-${env}.bin`);
const temp = resolve(root, '.pio', 'ota-package');
const partitionSize = 0x170000;
const magic = Buffer.from('ANTUIBR3');

if (env !== 'esp32dev') throw new Error(`Unsupported release environment: ${env}`);
mkdirSync(outDir, { recursive: true }); mkdirSync(temp, { recursive: true });
const pio = resolve(process.env.USERPROFILE, '.platformio', 'penv', 'Scripts', 'pio.exe');
const trustBundle = spawnSync('py', ['-3', resolve(root, 'tools', 'convert_github_trust_bundle.py')], { cwd: root, stdio: 'inherit', shell: false });
if (trustBundle.status !== 0) throw new Error('GitHub CA bundle generation failed');
const build = spawnSync(pio, ['run', '-e', env], { cwd: root, stdio: 'inherit', shell: false });
if (build.status !== 0) throw new Error(`PlatformIO build failed: ${env}`);
const image = readFileSync(firmware);

const appJs = resolve(temp, 'app.min.js');
const unchangedUiFiles = new Map([
  ['setup.html', '5471ebbd5ae429e56bf147512eb421d7780c0add3bfc54c8ce6cb2f76495d0ea'],
  ['logo.png', 'df0aa9b060b41794e52c8d5d1261f31f049bcd16cdf4fb818f0d42934735ab8a'],
  ['favicon.ico', '735c0feb9ce1c1da95623079740d1442a2330c1b49f443629fb30be420f22154'],
]);
for (const [name, expectedHash] of unchangedUiFiles) {
  const actualHash = createHash('sha256').update(readFileSync(resolve(root, 'data', name))).digest('hex');
  if (actualHash !== expectedHash) throw new Error(`data/${name} changed but is not in the OTA bundle. Extend the bundle before releasing.`);
}
const npxCli = join(dirname(process.execPath), 'node_modules', 'npm', 'bin', 'npx-cli.js');
const terser = spawnSync(process.execPath, [npxCli, '--yes', 'terser@5.44.1', resolve(root, 'data', 'app.js'), '--compress', '--mangle', '--toplevel', '--ecma', '2020', '--output', appJs], { cwd: root, stdio: 'inherit', shell: false });
if (terser.status !== 0) throw new Error('Pinned Terser minification failed');
const syntax = spawnSync(process.execPath, ['--check', appJs], { cwd: root, stdio: 'inherit', shell: false });
if (syntax.status !== 0) throw new Error('Minified browser application failed its JavaScript syntax check');

const brotli = data => brotliCompressSync(data, { params: { [zc.BROTLI_PARAM_QUALITY]: 11, [zc.BROTLI_PARAM_MODE]: zc.BROTLI_MODE_TEXT } });
const uiAssets = [
  readFileSync(resolve(root, 'data', 'index.html')),
  readFileSync(appJs),
  readFileSync(resolve(root, 'data', 'responsive.css')),
  readFileSync(resolve(root, 'data', 'style.css')),
];
const assets = [
  ...uiAssets.map(brotli),
  ...uiAssets.map(data => gzipSync(data, { level: 9, mtime: 0 })),
];
const encodedAssetGroups = [
  { encoding: 'Brotli', decode: brotliDecompressSync, items: assets.slice(0, 4) },
  { encoding: 'gzip', decode: gunzipSync, items: assets.slice(4) },
];
for (const group of encodedAssetGroups) {
  group.items.forEach((asset, i) => { if (!group.decode(asset).equals(uiAssets[i])) throw new Error(`${group.encoding} asset verification failed: ${i}`); });
}
/* The first four entries are preferred Brotli; the second four are browser-compatible gzip. */
const payloadBytes = assets.reduce((n, a) => n + a.length, 0);
const footerSize = 76;
if (image.length + payloadBytes + footerSize > partitionSize) {
  throw new Error(`Firmware plus complete UI does not fit: app=${image.length}, UI=${payloadBytes}, footer=${footerSize}, slot=${partitionSize}`);
}

const packageData = Buffer.alloc(partitionSize, 0xff);
image.copy(packageData, 0);
let cursor = image.length;
const entries = assets.map(a => { const entry = { offset: cursor, size: a.length }; a.copy(packageData, cursor); cursor += a.length; return entry; });

function crc32(data) {
  let crc = 0xffffffff;
  for (const value of data) { crc ^= value; for (let bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ (0xedb88320 & -(crc & 1)); }
  return (crc ^ 0xffffffff) >>> 0;
}
const payload = packageData.subarray(entries[0].offset, cursor);
const footer = packageData.subarray(partitionSize - footerSize);
magic.copy(footer, 0);
let pos = 8;
for (const entry of entries) { footer.writeUInt32LE(entry.offset, pos); footer.writeUInt32LE(entry.size, pos + 4); pos += 8; }
footer.writeUInt32LE(crc32(payload), pos);

writeFileSync(packagePath, packageData);
if (!packageData.subarray(partitionSize - footerSize, partitionSize).subarray(0, 8).equals(magic)) throw new Error('OTA footer verification failed');
if (crc32(packageData.subarray(entries[0].offset, cursor)) !== footer.readUInt32LE(72)) throw new Error('OTA payload checksum verification failed');
console.log(`OTA package: ${packagePath}`);
console.log(`Firmware ${image.length} B; UI Brotli ${assets.slice(0, 4).reduce((n, a) => n + a.length, 0)} B + gzip ${assets.slice(4).reduce((n, a) => n + a.length, 0)} B; slot ${partitionSize} B; remaining ${partitionSize - image.length - payloadBytes - footerSize} B`);
