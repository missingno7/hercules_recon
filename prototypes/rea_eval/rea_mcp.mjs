// Minimal MCP stdio client for REA: keeps one Ghidra session alive across calls.
// Usage: node rea_mcp.mjs <rea.mjs> <plan.json> <out.jsonl>
// plan.json: [{"tool": "...", "args": {...}, "label": "..."}, ...]
import { spawn } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import readline from 'node:readline';

const [reaPath, planPath, outPath] = process.argv.slice(2);
// "@file:<relative path>" string values are resolved to absolute paths (relative to cwd).
const plan = JSON.parse(fs.readFileSync(planPath, 'utf8'), (k, v) =>
  typeof v === 'string' && v.startsWith('@file:') ? path.resolve(v.slice(6)) : v);
const out = fs.createWriteStream(outPath);
const child = spawn(process.execPath, [reaPath, '--mcp'], { stdio: ['pipe', 'pipe', 'pipe'], env: process.env });
child.stderr.on('data', () => {});
const rl = readline.createInterface({ input: child.stdout });
const pending = new Map();
let nextId = 1;
rl.on('line', (line) => {
  let msg;
  try { msg = JSON.parse(line); } catch { return; }
  if (msg.id !== undefined && pending.has(msg.id)) { pending.get(msg.id)(msg); pending.delete(msg.id); }
});
function rpc(method, params) {
  const id = nextId++;
  child.stdin.write(JSON.stringify({ jsonrpc: '2.0', id, method, params }) + '\n');
  return new Promise((res) => pending.set(id, res));
}
const t0 = Date.now();
const init = await rpc('initialize', { protocolVersion: '2025-06-18', capabilities: {}, clientInfo: { name: 'hercules-rea-probe', version: '0' } });
child.stdin.write(JSON.stringify({ jsonrpc: '2.0', method: 'notifications/initialized' }) + '\n');
out.write(JSON.stringify({ label: 'initialize', ms: Date.now() - t0, result: init.result?.serverInfo }) + '\n');
for (const step of plan) {
  const s = Date.now();
  const r = step.tool === '__list_tools'
    ? await rpc('tools/list', {})
    : await rpc('tools/call', { name: step.tool, arguments: step.args || {} });
  const rec = { label: step.label || step.tool, tool: step.tool, args: step.args, ms: Date.now() - s };
  if (r.error) rec.error = r.error;
  else if (step.tool === '__list_tools') rec.tools = r.result.tools;
  else {
    rec.isError = r.result.isError || false;
    rec.text = (r.result.content || []).filter((c) => c.type === 'text').map((c) => c.text).join('\n');
    if (r.result.structuredContent) rec.structured = r.result.structuredContent;
  }
  out.write(JSON.stringify(rec) + '\n');
  process.stderr.write(`${rec.label}: ${rec.ms}ms${rec.isError ? ' ERROR' : ''}\n`);
}
out.end();
child.stdin.end();
child.kill();
process.exit(0);
