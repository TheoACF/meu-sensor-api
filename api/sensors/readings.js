// api/sensors/readings.js
let lastReceived = null; // memória volátil (some ao escalar/reiniciar)

export default async function handler(req, res) {
  // Permite GET (debug) e POST (produção)
  if (req.method === "GET") {
    return res.status(200).json({ ok: true, lastReceived, serverTime: Date.now() });
  }
  if (req.method !== "POST") {
    res.setHeader("Allow", "GET, POST");
    return res.status(405).json({ ok: false, error: "Use GET or POST" });
  }

  // Lê corpo cru SEM depender de parser automático
  let raw = "";
  try {
    for await (const chunk of req) raw += chunk;
  } catch (e) {
    console.warn("read error:", e);
    lastReceived = { raw, note: "stream read error", at: Date.now() };
    console.log("ESP payload:", lastReceived);
    return res.status(200).json({ ok: true, received: lastReceived, serverTime: Date.now() });
  }

  // Tenta interpretar como JSON; se falhar, guarda como texto
  let parsed = null;
  try { parsed = raw ? JSON.parse(raw) : null; } catch {}

  lastReceived = parsed ?? { raw, at: Date.now() };

  // 🔎 Loga nos Logs do Vercel (Deployments → Logs)
  console.log("ESP payload:", lastReceived);

  return res.status(200).json({ ok: true, received: lastReceived, serverTime: Date.now() });
}
