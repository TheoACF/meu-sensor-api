// api/sensors/readings.js
export default async function handler(req, res) {
  // Aceita só POST
  if (req.method !== "POST") {
    res.setHeader("Allow", "POST");
    return res.status(405).json({ ok: false, error: "Use POST" });
  }

  // Lê o corpo cru SEM depender de parser automático
  let raw = "";
  try {
    for await (const chunk of req) raw += chunk;
  } catch (e) {
    // se algo der errado até aqui, ainda assim responde 200 com o que conseguiu
    return res.status(200).json({ ok: true, received: { raw, note: "stream read error" } });
  }

  // Tenta interpretar como JSON; se falhar, devolve como texto
  let parsed = null;
  try {
    parsed = raw ? JSON.parse(raw) : null;
  } catch {
    // fica como texto mesmo
  }

  return res.status(200).json({
    ok: true,
    received: parsed ?? { raw },
    serverTime: Date.now()
  });
}
