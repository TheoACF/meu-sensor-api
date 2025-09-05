// Arquivo: /api/sensors/readings.js

export default function handler(req, res) {
  // Verifica se o método da requisição é POST
  if (req.method === 'POST') {
    // Pega os dados enviados no corpo da requisição
    const data = req.body;

    // Mostra os dados recebidos nos logs da Vercel (ótimo para depurar!)
    console.log('Dados recebidos do sensor:', data);

    // Responde ao ESP8266 com status 200 (Sucesso)
    res.status(200).json({ 
      status: "sucesso", 
      message: "Dados recebidos",
      dadosRecebidos: data 
    });
  } else {
    // Se não for POST, responde que o método não é permitido
    res.setHeader('Allow', ['POST']);
    res.status(405).end(`Método ${req.method} não permitido.`);
  }
}
