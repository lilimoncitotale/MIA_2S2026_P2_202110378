import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { listarDiscos } from '../api';

export default function DiskSelector() {
  const [discos, setDiscos] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const navigate = useNavigate();

  useEffect(() => {
    async function cargar() {
      try {
        const data = await listarDiscos();
        setDiscos(data);
      } catch (e) {
        setError('Error al cargar discos');
      }
      setLoading(false);
    }
    cargar();
  }, []);

  if (loading) return <div style={{ textAlign: 'center', padding: '3rem' }}>Cargando discos...</div>;

  return (
    <div style={{ padding: '2rem' }}>
      <h2>💾 Seleccione el disco que desea visualizar</h2>
      
      {error && <p style={{ color: 'red' }}>{error}</p>}
      
      {discos.length === 0 ? (
        <p>No hay discos disponibles. Cree uno con el comando MKDISK.</p>
      ) : (
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(200px, 1fr))', gap: '1.5rem', marginTop: '2rem' }}>
          {discos.map((disco, idx) => (
            <div
              key={idx}
              onClick={() => navigate(`/particiones?path=${encodeURIComponent(disco.path)}`)}
              style={{
                padding: '1.5rem',
                background: 'white',
                border: '1px solid #ddd',
                borderRadius: '8px',
                cursor: 'pointer',
                textAlign: 'center',
                transition: 'transform 0.2s'
              }}
              onMouseEnter={(e) => e.currentTarget.style.transform = 'translateY(-4px)'}
              onMouseLeave={(e) => e.currentTarget.style.transform = 'translateY(0)'}
            >
              <div style={{ fontSize: '3rem' }}>💿</div>
              <div style={{ fontWeight: 'bold', marginTop: '0.5rem' }}>{disco.name}</div>
              <div style={{ fontSize: '0.85rem', color: '#666', marginTop: '0.5rem' }}>
                {(disco.size / (1024*1024)).toFixed(2)} MB
              </div>
              <div style={{ fontSize: '0.85rem', color: '#666' }}>
                Fit: {disco.fit}
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}