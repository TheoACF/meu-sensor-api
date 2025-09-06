export default async function handler(req, res) {
  if (req.method !== "POST") {
    res.setHeader("Allow", "POST");
    return res.status(405).json({ ok: false, error: "Use POST" });
  }

  try {
    const body = req.body ?? {};
    console.log("ESP payload recebido:", body);

    return res.status(200).json({
      ok: true,
      received: body,
      serverTime: Date.now()
    });
  } catch (e) {
    return res.status(400).json({ ok: false, error: String(e) });
  }
}
