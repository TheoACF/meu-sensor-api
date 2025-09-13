// api/data.js
import { sql } from '@vercel/postgres';

export default async function handler(req, res) {
  if (req.method !== 'GET') {
    res.setHeader('Allow', 'GET');
    return res.status(405).json({ error: 'Method Not Allowed, please use GET' });
  }

  try {
    // Busca os 100 registros mais recentes, ordenados do mais novo para o mais antigo
    const { rows } = await sql`
      SELECT 
        sensor_id, 
        temperature, 
        humidity, 
        rssi,
        created_at
      FROM readings 
      ORDER BY created_at DESC 
      LIMIT 100;
    `;
    
    return res.status(200).json(rows);
  } catch (error) {
    console.error('Database query error:', error);
    return res.status(500).json({ error: 'Failed to fetch data from database' });
  }
}
