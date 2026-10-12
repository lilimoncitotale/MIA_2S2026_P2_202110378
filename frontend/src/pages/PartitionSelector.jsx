import React, { useState, useEffect } from 'react';
import { useSearchParams, useNavigate } from 'react-router-dom';
import { infoDisco } from '../api';

export default function PartitionSelector() {
  const [searchParams] = useSearchParams();
  const path = searchParams.get('path');
  const navigate = useNavigate();
  
  const [disco, setDisco] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    async function cargar() {
      try {
        const data = await infoDisco(path);
        setDisco(data);
      } catch (e) {
        console.error(e);
      }
      setLoading(false);
    }
    if (path) cargar();
  }, [path]);

  if (loading) return <div style={{ padding: '3rem', textAlign: 'center' }}>Cargando...</div>;
  if (!disco) return <div style={{ padding: '3rem', textAlign: 'center' }}>Disco no encontrado</div>;

  return (
    <div style={{ padding: '2rem' }}>
      <h2>📂 Seleccione la partición que desea visualizar</h2>
      <p style={{ color: '#666' }}>Disco: {disco.path}</p>

      {disco.partitions?.length === 0 ? (
        <p>Este disco no tiene particiones.</p>
      ) : (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(220px, 1fr))', gap: '1.5rem', marginTop: '2rem' }}>
          {disco.partitions?.map((part, idx) => (
            <div
              key={idx}
              onClick={() => {
                if (part.status === 'Montada' && part.id) {
                  navigate(`/explorador?id=${part.id}`);
                } else {
                  alert('La partición no está montada. Use el comando MOUNT primero.');
                }
              }}
              style={{
                padding: '1.5rem',
                background: part.status === 'Montada' ? '#f0f9ff' : 'white',
                border: part.status === 'Montada' ? '2px solid #2563eb' : '1px solid #ddd',
                borderRadius: '8px',
                cursor: 'pointer',
                textAlign: 'center'
              }}
            >
              <div style={{ fontSize: '3rem' }}>🗂️</div>
              <div style={{ fontWeight: 'bold' }}>{part.name}</div>
              <div style={{ fontSize: '0.85rem', color: '#666' }}>
                {part.type} · {(part.size / (1024*1024)).toFixed(2)} MB
              </div>
              <div style={{ fontSize: '0.85rem', color: part.status === 'Montada' ? '#16a34a' : '#dc2626', fontWeight: 'bold' }}>
                {part.status}
              </div>
              {part.id && (
                <div style={{ fontSize: '0.85rem', color: '#666' }}>
                  ID: {part.id}
                </div>
              )}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}