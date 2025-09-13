// api/sensors/readings.js
import { sql } from '@vercel/postgres';

// Função para garantir que a tabela exista.
// Ela será executada na primeira vez que a API for chamada.
async function ensureTableExists() {
  try {
    await sql`
      CREATE TABLE IF NOT EXISTS readings (
        id SERIAL PRIMARY KEY,
        sensor_id VARCHAR(50) NOT NULL,
        temperature REAL,
        humidity REAL,
        rssi INTEGER,
        uptime_seconds BIGINT,
        has_error BOOLEAN DEFAULT FALSE,
        created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
      );
    `;
  } catch (error) {
    console.error('Error creating table:', error);
    // Não impede a execução, mas loga o erro.
  }
}

// Executa a verificação da tabela uma vez quando a função "acorda".
const tableCheck = ensureTableExists();

export default async function handler(req, res) {
  // Permite GET apenas para um simples "estou vivo"
  if (req.method === 'GET') {
    return res.status(200).json({ status: 'API is running' });
  }

  // Apenas POST é permitido para receber dados
  if (req.method !== 'POST') {
    res.setHeader('Allow', 'POST');
    return res.status(405).json({ error: 'Method Not Allowed, please use POST' });
  }

  // Espera a verificação da tabela terminar antes de continuar
  await tableCheck;

  let raw = '';
  try {
    for await (const chunk of req) {
      raw += chunk;
    }
  } catch (e) {
    console.error('Stream read error:', e);
    return res.status(400).json({ error: 'Error reading request body' });
  }
  
  let data;
  try {
    data = JSON.parse(raw);
  } catch (e) {
    console.warn('Invalid JSON payload:', raw);
    return res.status(400).json({ error: 'Invalid JSON format' });
  }

  // Validação básica dos dados recebidos
  if (!data || !data.sensorId) {
    return res.status(400).json({ error: 'Missing required field: sensorId' });
  }

  // Inserindo no banco de dados
  try {
    await sql`
      INSERT INTO readings (sensor_id, temperature, humidity, rssi, uptime_seconds, has_error)
      VALUES (
        ${data.sensorId},
        ${data.temperature ?? null},
        ${data.humidity ?? null},
        ${data.rssi ?? null},
        ${data.uptime ?? null},
        ${'error' in data}
      );
    `;
    console.log('ESP payload saved:', data);
    return res.status(201).json({ success: true, received: data });
  } catch (error) {
    console.error('Database insert error:', error);
    return res.status(500).json({ error: 'Could not save data to database' });
  }
}
