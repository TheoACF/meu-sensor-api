// api/sensors/readings.js
export default async function handler(req, res) {
  if (req.method !== "POST") {
    res.setHeader("Allow", "POST");
    return res.status(405).json({ ok: false, error: "Use POST" });
  }
  const body = req.body ?? {};
  console.log("ESP payload:", body);
  return res.status(200).json({ ok: true, received: body, serverTime: Date.now() });
}
