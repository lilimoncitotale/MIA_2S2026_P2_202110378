import React, { useState, useEffect } from 'react';
import { listarJournal } from '../api';
import { useSession } from '../context/SessionContext';

export default function Journal() {
  const { session } = useSession();
  const [id, setId] = useState(session?.id || '');
  const [entries, setEntries] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');

  const cargar = async () => {
    if (!id) return;
    setLoading(true);
    setError('');
    try {
      const data = await listarJournal(id);
      if (data.error) {
        setError(data.error);
        setEntries([]);
      } else {
        setEntries(data.entries || []);
      }
    } catch (e) {
      setError('Error al consultar journal');
    }
    setLoading(false);
  };

  useEffect(() => {
    if (session?.id) {
      setId(session.id);
      cargar();
    }
  }, [session]);

  return (
    <div style={{ padding: '2rem' }}>
      <h2>📓 Journaling</h2>

      <div style={{ display: 'flex', gap: '0.5rem', marginTop: '1rem' }}>
        <input
          type="text"
          value={id}
          onChange={(e) => setId(e.target.value)}
          placeholder="ID de partición (781A)"
          style={{ padding: '0.5rem', flex: 1, border: '1px solid #ccc', borderRadius: '4px' }}
        />
        <button onClick={cargar}
          style={{ padding: '0.5rem 1.5rem', background: '#2563eb', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer' }}>
          Consultar
        </button>
      </div>

      {error && <p style={{ color: 'red', marginTop: '1rem' }}>❌ {error}</p>}

      {loading ? (
        <p>Cargando...</p>
      ) : entries.length > 0 ? (
        <table style={{ width: '100%', marginTop: '2rem', borderCollapse: 'collapse' }}>
          <thead>
            <tr style={{ background: '#16a34a', color: 'white' }}>
              <th style={{ padding: '0.75rem', textAlign: 'left' }}>Operación</th>
              <th style={{ padding: '0.75rem', textAlign: 'left' }}>Path</th>
              <th style={{ padding: '0.75rem', textAlign: 'left' }}>Contenido</th>
              <th style={{ padding: '0.75rem', textAlign: 'left' }}>Fecha</th>
            </tr>
          </thead>
          <tbody>
            {entries.map((e, idx) => (
              <tr key={idx} style={{ borderBottom: '1px solid #ddd', background: idx % 2 === 0 ? '#f8fafc' : 'white' }}>
                <td style={{ padding: '0.75rem' }}>
                  <span style={{ background: '#dbeafe', padding: '0.25rem 0.5rem', borderRadius: '4px', fontSize: '0.85rem' }}>
                    {e.operation}
                  </span>
                </td>
                <td style={{ padding: '0.75rem', fontFamily: 'monospace' }}>{e.path}</td>
                <td style={{ padding: '0.75rem', fontFamily: 'monospace', fontSize: '0.85rem' }}>{e.content}</td>
                <td style={{ padding: '0.75rem', fontSize: '0.85rem', color: '#666' }}>{e.date}</td>
              </tr>
            ))}
          </tbody>
        </table>
      ) : (
        <p style={{ marginTop: '2rem', color: '#666' }}>No hay transacciones registradas.</p>
      )}
    </div>
  );
}