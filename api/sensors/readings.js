// api/sensors/readings.js
export default async function handler(req, res) {
  if (req.method !== "POST") {
    res.setHeader("Allow", "POST");
    return res.status(405).json({ ok: false, error: "Use POST" });
  }

  try {
    let body = req.body;

    // Se veio string, tenta converter
    if (typeof body === "string" && body.length) {
      try { body = JSON.parse(body); } catch { /* fica string mesmo */ }
    }

    // Se veio undefined (sem parse automático), lê o raw
    if (body == null) {
      const chunks = [];
      for await (const ch of req) chunks.push(ch);
      const raw = Buffer.concat(chunks).toString("utf8");
      try { body = JSON.parse(raw); } catch { body = { raw }; }
    }

    return res.status(200).json({
      ok: true,
      received: body,
      serverTime: Date.now()
    });
  } catch (e) {
    return res.status(400).json({ ok: false, error: String(e) });
  }
}
